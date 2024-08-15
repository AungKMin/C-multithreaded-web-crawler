#include "curl_xml.h"

/**
 * @brief set options of curl easy handle
 * @param curl_handle CURL*: (pointer to) already-initialized curl easy handle to configure
 * @param ptr RECV_BUF*: (pointer to) user data needed by the curl write call back function
 * @param url const char*: target url to crawl
 * @return a valid CURL handle upon sucess; NULL otherwise
 * note the caller is responsible for cleaning the returned curl handle
 */
CURL *easy_handle_config(CURL *curl_handle, RECV_BUF *ptr, const char *url)
{
    if (ptr == NULL || url == NULL)
    {
        return NULL;
    }

    // init user defined call back function buffer
    if (recv_buf_init(ptr, BUF_SIZE) != 0)
    {
        return NULL;
    }

    // specify URL to get
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // register write call back function to process received data
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl);
    // user defined data structure passed to the call back function
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)ptr);

    // register header call back function to process received header data
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
    // user defined data structure passed to the call back function
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)ptr);

    /// ome servers require a user-agent field
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, CURL_USER_AGENT_FIELD);

    // follow HTTP 3XX redirects
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    // continue to send authentication credentials when following locations
    curl_easy_setopt(curl_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    // max numbre of redirects to follow sets to 5
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    // supports all built-in encodings
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    // Enable the cookie engine without reading any initial cookies
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    // allow whatever auth the proxy speaks
    curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    // allow whatever auth the server speaks
    curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

    return curl_handle;
}

/**
 * @brief process a downloaded html page: get all urls from the page and push it onto stack
 * @param curl_handle CURL*: (pointer to) curl handler that was used to access the url
 * @param p_recv_buf RECV_BUF*: (pointer to) buffer that contains the received data
 * @param content_type int*: (pointer to) int to be set with content type code
 * @param stack STACK*: (pointer to) stack that will be populated with further urls to crawl
 * @return 0 on success; non-zero otherwise
 */
int process_html(CURL *curl_handle, RECV_BUF *p_recv_buf, int *content_type, STACK *stack)
{
    *content_type = HTML;

    int follow_relative_link = 1;
    char *url = NULL;

    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
    find_http(p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url, stack);
    return 0;
}

/**
 * @brief check if a png is a valid png
 * @param buf uint8_t*: (pointer to) memory containing the supposed png image
 * @param n size_t: size of the memory
 * @return true if memory contains a valid png; false otherwise
 * @details
 * The check is derived from the png specification: https://www.w3.org/TR/png/
 */
bool is_png(uint8_t *buf, size_t n)
{
    if (n < 8)
    {
        return false;
    }

    if ((buf[0] == 0x89) &&
        (buf[1] == 0x50) &&
        (buf[2] == 0x4E) &&
        (buf[3] == 0x47) &&
        (buf[4] == 0x0D) &&
        (buf[5] == 0x0A) &&
        (buf[6] == 0x1A) &&
        (buf[7] == 0x0A))
    {
        return true;
    }

    return false;
}

/**
 * @brief process a downloaded png: check if it's a valid png
 * @param curl_handle CURL*: (pointer to) curl handler that was used to access the url
 * @param p_recv_buf RECV_BUF*: (pointer to) buffer that contains the received data
 * @param content_type int*: (pointer to) int to be set with content type code
 * @return 0 on success; non-zero otherwise
 */
int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf, int *content_type)
{
    // effective url
    char *eurl = NULL;
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);

    if (is_png((uint8_t *)p_recv_buf->buf, p_recv_buf->size))
    {
        *content_type = VALID_PNG;
    }
    else
    {
        *content_type = INVALID_PNG;
    }

    return 0;
}

/**
 * @brief process the downloaded content data
 * @param curl_handle CURL*: (pointer to) curl handler that was used to access the url
 * @param p_recv_buf RECV_BUF*: (pointer to) buffer that contains the received data
 * @param content_type int*: (pointer to) int to be set with content type code
 * @param stack STACK*: (pointer to) stack that will be populated with further urls to crawl
 * @param response_code_p long*: (pointer to) int to be set with the response code
 * @return 0 on success; non-zero otherwise
 * @details
 * if url points to a HTML page, populate stack with urls linked on the page
 * if url points to a png, check if it's a valid png
 * set the content type and response code via the appropriate pointers
 */
int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf, int *content_type, STACK *stack, long *response_code_p)
{
    CURLcode res;

    // get response code, and return if it's a fail
    long response_code;
    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    *response_code_p = response_code;
    if (response_code >= BAD_REQUESTS)
    {
        return 1;
    }

    // get content, and handle differently depending on content type
    char *ct = NULL;
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if (res != CURLE_OK || ct == NULL)
    {
        return 2;
    }
    if (strstr(ct, CT_HTML))
    {
        return process_html(curl_handle, p_recv_buf, content_type, stack);
    }
    else if (strstr(ct, CT_PNG))
    {
        return process_png(curl_handle, p_recv_buf, content_type);
    }

    return 0;
}

/**
 * @brief crawl specified url and process the downloaded data
 * @param curl_handle CURL*: (pointer to) the curl handler that will be used to process the url
 * @param seed_url char*: string containing the url to crawl
 * @param content_type int*: (pointer to) int to be set with content type code
 * @param stack STACK*: (pointer to) stack that will be populated with further urls to crawl
 * @param response_code_p long*: (pointer to) int to be set with the response code
 * @return 0 on success; non-zero otherwise
 * @details
 * if url points to a HTML page, populate stack with urls linked on the page
 * if url points to a png, check if it's a valid png
 * set the content type and response code via the appropriate pointers
 */
int process_url(CURL *curl_handle, char *seed_url, int *content_type, STACK *stack, long *response_code_p)
{
    // set default response code to failure (if nothing fails, the code will be set later)
    *response_code_p = INTERNAL_SERVER_ERRORS;

    // configure the easy curl handle
    char url[URL_LENGTH];
    RECV_BUF recv_buf;
    strcpy(url, seed_url);
    curl_handle = easy_handle_config(curl_handle, &recv_buf, url);
    if (curl_handle == NULL)
    {
        fprintf(stderr, "Curl configuration failed. Exiting...\n");
        curl_global_cleanup();
        abort();
    }

    // process the url
    CURLcode res;
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        recv_buf_cleanup(&recv_buf);
        return 1;
    }

    // process the data from the url
    process_data(curl_handle, &recv_buf, content_type, stack, response_code_p);

    // clean up data buffer
    recv_buf_cleanup(&recv_buf);
    return 0;
}

/**
 * @brief returns whether the response code is not an error (i.e. okay or redirect)
 * @param response_code long: response_code in question
 * @return true if response code is crawlable (not an error); false otherwise
 */
bool is_processable_response(long response_code)
{
    return (response_code >= OK_REQUESTS && response_code <= OK_REQUESTS + CODE_RANGE) || (response_code >= REDIRECT_REQUESTS && response_code <= REDIRECT_REQUESTS + CODE_RANGE);
}

/**
 * @brief cURL header call back function to extract image sequence number from
 *        http header data. An example header for image part n (assume n = 2) is:
 *        X-Ece252-Fragment: 2
 * @param p_recv char*: (pointer to) header data delivered by cURL
 * @param size size_t: number of data elements
 * @param nmemb size_t: size of one data element
 * @param userdata void*: (pointer to) buffer containing user-defined data
 *                        structure used for extracting sequence number
 * @return total size of header data received
 * @details
 * cURL documentation: https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;

    if (realsize > strlen(ECE252_HEADER) &&
        strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0)
    {
        // extract image sequence number
        p->seq = atoi(p_recv + strlen(ECE252_HEADER));
    }
    return realsize;
}

/**
 * @brief cURL write callback function that saves a copy of received data
 * @param p_recv char*: (pointer to) memory that will be populated with received data
 * @param size size_t: number of data elements
 * @param nmemb size_t: size of one data element
 * @param p_userdata void*: (pointer to) user data buffer that can be acccessed outside CURL;
 *                          this will be populated with the web page (or png file)
 * @return total size of data received
 * @details
 * cURL documentation: https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
 */
size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;

    if (p->size + realsize + 1 > p->max_size)
    {
        // since received data is not 0 terminated, add one byte for terminating 0
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);
        char *q = realloc(p->buf, new_size);
        if (q == NULL)
        {
            // out of memory
            perror("realloc");
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    // copy data from libcurl into a buffer we can access later
    memcpy(p->buf + p->size, p_recv, realsize);
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

/**
 * @brief initialize data structure used for downloading data via cURL
 * @param ptr RECV_BUF*: a pointer to uninitialized memory
 * @param max_size size_t: maximum expected size of data to download
 * @return 0 on success; non-zero otherwise
 * @details
 *  note that if the size of downloaded data is larger than the max_size,
 *  we will reallocate to accommodate
 */
int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;

    if (ptr == NULL)
    {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL)
    {
        return 2;
    }

    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    // a valid sequence number should be positive
    ptr->seq = -1;
    return 0;
}

/**
 * @brief clean up data structure used for downloading data via cURL: deallocate memory
 * @param ptr RECV_BUF*: (pointer to) RECV_BUF to clean
 * @return 0 on success; non-zero otherwise
 */
int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL)
    {
        return 1;
    }

    if (ptr->buf != NULL)
    {
        free(ptr->buf);
        ptr->buf = NULL;
    }

    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

/**
 * @brief clean up all cURL related data structures
 * @param curl CURL*: (pointer to) curl easy handle to clean up
 * @param ptr RECV_BUF*: a pointer to data structure used for downloading data via cURL
 */
void cleanup(CURL *curl, RECV_BUF *ptr)
{
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    recv_buf_cleanup(ptr);
}

/**
 * @brief get html document from data
 * @param buf char*: (pointer to) memory containing document data
 * @param size int: size of data
 * @param url char*: url string of the html document
 * @return document pointer if successful; NULL otherwise
 */
htmlDocPtr mem_getdoc(char *buf, int size, const char *url)
{
    int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR |
               HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    htmlDocPtr doc = htmlReadMemory(buf, size, url, NULL, opts);

    if (doc == NULL)
    {
        printf("%s\n", url);
        fprintf(stderr, "Document not parsed successfully.\n");
        return NULL;
    }

    return doc;
}

/**
 * @brief get nodes on the xml page
 * @param doc xmlDocPtr: xml document to get nodes from
 * @param xpath xmlChar*: xpath
 * @return nodes on success; NULL otherwise
 */
xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL)
    {
        printf("Error in xmlXPathNewContext\n");
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL)
    {
        printf("Error in xmlXPathEvalExpression\n");
        return NULL;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        xmlXPathFreeObject(result);
        printf("No result\n");
        return NULL;
    }
    return result;
}

/**
 * @brief get all urls on the xml web page and push them onto stack
 * @param buf char*: (pointer to) buffer that contains the HTML web page
 * @param size int: size of the buffer
 * @param follow_relative_links int: 1 if we're following relative links and 0 otherwise
 * @param base_url const char*: base url of the page
 * @param stack STACK*: (pointer to) stack that will be populated with further urls to crawl
 * @return 0 on success; non-zero otherwise
 */
int find_http(char *buf, int size, int follow_relative_links, const char *base_url, STACK *stack)
{
    int i;
    htmlDocPtr doc;
    xmlChar *xpath = (xmlChar *)"//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    xmlChar *href;

    if (buf == NULL)
    {
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);

    result = getnodeset(doc, xpath);
    if (result)
    {
        nodeset = result->nodesetval;
        for (i = 0; i < nodeset->nodeNr; i++)
        {
            href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
            if (follow_relative_links)
            {
                xmlChar *old = href;
                href = xmlBuildURI(href, (xmlChar *)base_url);
                xmlFree(old);
            }
            if (href != NULL && !strncmp((const char *)href, "http", 4))
            {
                char *temp = malloc((strlen((char *)href) + 1) * sizeof(char));
                memset(temp, 0, (strlen((char *)href) + 1) * sizeof(char));
                sprintf(temp, "%s", href);
                push_stack(stack, (char *)temp);
                free(temp);
                temp = NULL;
            }
            xmlFree(href);
        }
        xmlXPathFreeObject(result);
    }

    xmlFreeDoc(doc);

    return 0;
}