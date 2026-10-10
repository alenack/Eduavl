/* Cross-platform (Windows Winsock + Linux/POSIX) AVL student server */
#ifdef _WIN32
  #define _WIN32_WINNT 0x0600
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <process.h>
  typedef int sock_len_t;
  #define PATH_SEP "\\"
  #define LOCK_T CRITICAL_SECTION
  #define LOCK_INIT(l) InitializeCriticalSection(l)
  #define LOCK(l) EnterCriticalSection(l)
  #define UNLOCK(l) LeaveCriticalSection(l)
  #define LOCK_DESTROY(l) DeleteCriticalSection(l)
  #define WORKER_RET unsigned __stdcall
  #define WORKER_DONE 0
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <pthread.h>
  #include <signal.h>
  #include <strings.h>
  #include <errno.h>
  typedef int SOCKET;
  typedef socklen_t sock_len_t;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
  #define MAX_PATH 4096
  #define PATH_SEP "/"
  #define closesocket close
  #define SD_BOTH SHUT_RDWR
  #define strnicmp strncasecmp
  #define LOCK_T pthread_mutex_t
  #define LOCK_INIT(l) pthread_mutex_init(l, NULL)
  #define LOCK(l) pthread_mutex_lock(l)
  #define UNLOCK(l) pthread_mutex_unlock(l)
  #define LOCK_DESTROY(l) pthread_mutex_destroy(l)
  #define WORKER_RET void *
  #define WORKER_DONE NULL
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "avl.h"

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 65536
#define JSON_BUFFER_SIZE 131072

/* Global AVL Tree state and thread synchronization */
static AVLNode *g_root = NULL;
static LOCK_T g_tree_lock;
static char g_last_rotation_log[256] = "System initialized. Tree is empty and balanced.";
static char g_base_dir[MAX_PATH] = "";

static void init_base_dir(void) {
#ifdef _WIN32
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) > 0) {
        char *last_slash = strrchr(exe_path, '\\');
        if (!last_slash) last_slash = strrchr(exe_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            strncpy(g_base_dir, exe_path, sizeof(g_base_dir) - 1);
            g_base_dir[sizeof(g_base_dir) - 1] = '\0';
        }
    }
#else
    /* On Linux/Docker the server runs from its project dir; "public/" is resolved relative to cwd */
    g_base_dir[0] = '\0';
#endif
}

/* Function prototypes */
static WORKER_RET client_worker(void *arg);
static void handle_request(SOCKET client_sock, const char *request, const char *body);
static void send_response(SOCKET client_sock, int status_code, const char *status_text,
                          const char *content_type, const char *body, size_t body_len);
static void send_file_response(SOCKET client_sock, const char *filepath, const char *content_type);
static bool parse_json_str(const char *json, const char *key, char *out, size_t max_len);
static bool parse_json_int(const char *json, const char *key, int *out);
static void seed_demo_students(void);
static void send_all(SOCKET s, const char *data, size_t len);

/* Helper to find a substring case-insensitively */
static const char* stristr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    size_t nlen = strlen(needle);
    if (nlen == 0) return haystack;
    while (*haystack) {
        if (strnicmp(haystack, needle, nlen) == 0) {
            return haystack;
        }
        haystack++;
    }
    return NULL;
}

/* Clean string trim helper */
static void trim_str(char *s) {
    if (!s) return;
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

/* Parse a string value for a given key from JSON */
static bool parse_json_str(const char *json, const char *key, char *out, size_t max_len) {
    if (!json || !key || !out || max_len == 0) return false;
    out[0] = '\0';

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = stristr(json, pattern);
    if (!pos) return false;

    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == ':') pos++;
    if (*pos != '"') return false;
    pos++; /* skip opening quote */

    size_t idx = 0;
    while (*pos && idx < max_len - 1) {
        if (*pos == '\\' && *(pos + 1) != '\0') {
            pos++;
            if (*pos == 'n') out[idx++] = '\n';
            else if (*pos == 't') out[idx++] = '\t';
            else if (*pos == 'r') out[idx++] = '\r';
            else out[idx++] = *pos;
        } else if (*pos == '"') {
            break;
        } else {
            out[idx++] = *pos;
        }
        pos++;
    }
    out[idx] = '\0';
    trim_str(out);
    return true;
}

/* Parse an integer value for a given key from JSON */
static bool parse_json_int(const char *json, const char *key, int *out) {
    if (!json || !key || !out) return false;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = stristr(json, pattern);
    if (!pos) return false;

    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == ':') pos++;

    char *endptr = NULL;
    long val = strtol(pos, &endptr, 10);
    if (endptr == pos) return false;

    *out = (int)val;
    return true;
}

/* Seed realistic student data for quick demonstration */
static void seed_demo_students(void) {
    StudentData samples[] = {
        {104, "Aarav Sharma", "Male", "2003-04-12", "45 Park Street, Delhi", "+91 9876543210", "Computer Science"},
        {102, "Ananya Verma", "Female", "2002-11-28", "12 Richmond Road, Bangalore", "ananya.v@univ.edu", "Data Science"},
        {108, "Rohan Iyer", "Male", "2003-08-19", "78 Anna Salai, Chennai", "+91 9811223344", "Artificial Intelligence"},
        {101, "Diya Patel", "Female", "2004-01-05", "23 SG Highway, Ahmedabad", "diya.p@univ.edu", "Cybersecurity"},
        {103, "Kabir Mehta", "Male", "2002-06-14", "9 Marine Drive, Mumbai", "+91 9845012345", "Software Engineering"},
        {106, "Pooja Nair", "Female", "2003-09-30", "55 MG Road, Kochi", "pooja.nair@univ.edu", "Information Tech"},
        {110, "Siddharth Das", "Male", "2002-03-22", "88 Salt Lake, Kolkata", "+91 9830099887", "Computer Science"},
        {105, "Meera Joshi", "Female", "2003-12-10", "16 FC Road, Pune", "meera.j@univ.edu", "Data Science"}
    };

    int count = sizeof(samples) / sizeof(samples[0]);
    for (int i = 0; i < count; i++) {
        AVLError status;
        char log[256] = {0};
        g_root = avl_insert(g_root, &samples[i], &status, log, sizeof(log));
        if (log[0] != '\0') {
            strncpy(g_last_rotation_log, log, sizeof(g_last_rotation_log) - 1);
            g_last_rotation_log[sizeof(g_last_rotation_log) - 1] = '\0';
        }
    }
}

/* Thread worker parameter structure */
typedef struct {
    SOCKET sock;
} ClientContext;

/* Thread worker to handle an incoming HTTP connection */
static WORKER_RET client_worker(void *arg) {
    ClientContext *ctx = (ClientContext*)arg;
    SOCKET client_sock = ctx->sock;
    free(ctx);

    char *buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        closesocket(client_sock);
        return WORKER_DONE;
    }

    int total_received = 0;
    int content_length = -1;
    int header_size = 0;

    /* Loop until headers and complete Content-Length body have arrived */
    while (total_received < BUFFER_SIZE - 1) {
        int n = recv(client_sock, buffer + total_received, BUFFER_SIZE - 1 - total_received, 0);
        if (n <= 0) break;
        total_received += n;
        buffer[total_received] = '\0';

        if (content_length < 0) {
            char *hdr_end = strstr(buffer, "\r\n\r\n");
            if (hdr_end) {
                header_size = (int)(hdr_end - buffer) + 4;
                const char *cl_pos = stristr(buffer, "Content-Length:");
                if (cl_pos) {
                    content_length = atoi(cl_pos + 15);
                } else {
                    content_length = 0;
                }

                /* Handle Expect: 100-continue if client requested it */
                if (stristr(buffer, "Expect: 100-continue")) {
                    const char *cont_resp = "HTTP/1.1 100 Continue\r\n\r\n";
                    send(client_sock, cont_resp, (int)strlen(cont_resp), 0);
                }
            }
        }

        if (content_length >= 0) {
            int body_received = total_received - header_size;
            if (body_received >= content_length) {
                break;
            }
        }
    }

    if (total_received > 0) {
        char *body = "";
        char *hdr_end = strstr(buffer, "\r\n\r\n");
        if (hdr_end) {
            body = hdr_end + 4;
        }
        handle_request(client_sock, buffer, body);
    }

    free(buffer);
    shutdown(client_sock, SD_BOTH);
    closesocket(client_sock);
    return WORKER_DONE;
}

/* Send all bytes (send() may write only part of a large buffer) */
static void send_all(SOCKET s, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = (int)send(s, data + sent, (int)(len - sent), 0);
        if (n <= 0) return;
        sent += (size_t)n;
    }
}

/* Send a formatted HTTP response */
static void send_response(SOCKET client_sock, int status_code, const char *status_text,
                          const char *content_type, const char *body, size_t body_len) {
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_text, content_type, body_len);

    send_all(client_sock, header, (size_t)header_len);
    if (body && body_len > 0) {
        send_all(client_sock, body, body_len);
    }
}

/* Send a static file from disk */
static void send_file_response(SOCKET client_sock, const char *filepath, const char *content_type) {
    char fullpath[MAX_PATH * 2];
    if (g_base_dir[0] != '\0') {
        snprintf(fullpath, sizeof(fullpath), "%s" PATH_SEP "%s", g_base_dir, filepath);
    } else {
        strncpy(fullpath, filepath, sizeof(fullpath) - 1);
        fullpath[sizeof(fullpath) - 1] = '\0';
    }

    FILE *f = fopen(fullpath, "rb");
    if (!f) {
        f = fopen(filepath, "rb");
    }

    if (!f) {
        char err[512];
        snprintf(err, sizeof(err), "{\"error\":\"File Not Found: %s\"}", filepath);
        send_response(client_sock, 404, "Not Found", "application/json", err, strlen(err));
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = (char*)malloc(size + 1);
    if (!data) {
        fclose(f);
        const char *err = "{\"error\":\"Internal Server Error\"}";
        send_response(client_sock, 500, "Internal Server Error", "application/json", err, strlen(err));
        return;
    }

    size_t read_bytes = fread(data, 1, size, f);
    fclose(f);
    data[read_bytes] = '\0';

    send_response(client_sock, 200, "OK", content_type, data, read_bytes);
    free(data);
}

/* Main HTTP request dispatcher */
static void handle_request(SOCKET client_sock, const char *request, const char *body) {
    char method[16] = {0};
    char uri[256] = {0};

    if (sscanf(request, "%15s %255s", method, uri) < 2) {
        const char *err = "{\"error\":\"Bad Request\"}";
        send_response(client_sock, 400, "Bad Request", "application/json", err, strlen(err));
        return;
    }

    /* Handle CORS preflight */
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(client_sock, 204, "No Content", "text/plain", "", 0);
        return;
    }

    /* Extract path and query params */
    char path[256];
    char query[256] = {0};
    char *qmark = strchr(uri, '?');
    if (qmark) {
        size_t path_len = qmark - uri;
        if (path_len >= sizeof(path)) path_len = sizeof(path) - 1;
        strncpy(path, uri, path_len);
        path[path_len] = '\0';
        strncpy(query, qmark + 1, sizeof(query) - 1);
        query[sizeof(query) - 1] = '\0';
    } else {
        strncpy(path, uri, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    }

    /* 1. Static Files */
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            send_file_response(client_sock, "public/index.html", "text/html; charset=utf-8");
            return;
        } else if (strcmp(path, "/style.css") == 0) {
            send_file_response(client_sock, "public/style.css", "text/css; charset=utf-8");
            return;
        } else if (strcmp(path, "/app.js") == 0) {
            send_file_response(client_sock, "public/app.js", "application/javascript; charset=utf-8");
            return;
        }
    }

    /* 2. API Routes */

    /* GET /api/stats */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/stats") == 0) {
        LOCK(&g_tree_lock);
        int count = avl_count(g_root);
        int height = avl_height(g_root);
        int bf = avl_balance_factor(g_root);
        bool balanced = avl_is_balanced(g_root);
        char rotation_esc[300];
        strncpy(rotation_esc, g_last_rotation_log, sizeof(rotation_esc) - 1);
        rotation_esc[sizeof(rotation_esc) - 1] = '\0';
        UNLOCK(&g_tree_lock);

        char resp[1024];
        snprintf(resp, sizeof(resp),
            "{\"node_count\":%d,\"height\":%d,\"balance_factor\":%d,\"is_balanced\":%s,\"last_rotation\":\"%s\"}",
            count, height, bf, balanced ? "true" : "false", rotation_esc);
        send_response(client_sock, 200, "OK", "application/json", resp, strlen(resp));
        return;
    }

    /* GET /api/students (In-Order traversal by default) */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/students") == 0) {
        char *json_buf = (char*)malloc(JSON_BUFFER_SIZE);
        if (!json_buf) {
            send_response(client_sock, 500, "Server Error", "application/json", "{\"error\":\"Memory allocation failed\"}", 34);
            return;
        }

        LOCK(&g_tree_lock);
        avl_traversal_to_json(g_root, TRAVERSAL_INORDER, json_buf, JSON_BUFFER_SIZE);
        UNLOCK(&g_tree_lock);

        send_response(client_sock, 200, "OK", "application/json", json_buf, strlen(json_buf));
        free(json_buf);
        return;
    }

    /* GET /api/students/{id} (Search by ID) */
    if (strcmp(method, "GET") == 0 && strncmp(path, "/api/students/", 14) == 0) {
        int student_id = atoi(path + 14);
        if (student_id <= 0) {
            const char *err = "{\"error\":\"Invalid Student ID requested.\"}";
            send_response(client_sock, 400, "Bad Request", "application/json", err, strlen(err));
            return;
        }

        LOCK(&g_tree_lock);
        AVLNode *found = avl_search(g_root, student_id);
        if (found) {
            char node_json[1024];
            avl_node_to_json(found, node_json, sizeof(node_json));
            UNLOCK(&g_tree_lock);
            send_response(client_sock, 200, "OK", "application/json", node_json, strlen(node_json));
        } else {
            UNLOCK(&g_tree_lock);
            char err[256];
            snprintf(err, sizeof(err), "{\"error\":\"Student not found with ID #%d.\"}", student_id);
            send_response(client_sock, 404, "Not Found", "application/json", err, strlen(err));
        }
        return;
    }

    /* GET /api/tree (Full hierarchical tree for visual node map) */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/tree") == 0) {
        char *json_buf = (char*)malloc(JSON_BUFFER_SIZE);
        if (!json_buf) {
            send_response(client_sock, 500, "Server Error", "application/json", "{\"error\":\"Out of memory\"}", 22);
            return;
        }

        LOCK(&g_tree_lock);
        avl_tree_to_json(g_root, json_buf, JSON_BUFFER_SIZE);
        UNLOCK(&g_tree_lock);

        send_response(client_sock, 200, "OK", "application/json", json_buf, strlen(json_buf));
        free(json_buf);
        return;
    }

    /* GET /api/traversal?type=inorder|preorder|postorder */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/traversal") == 0) {
        TraversalType ttype = TRAVERSAL_INORDER;
        if (strstr(query, "type=preorder")) {
            ttype = TRAVERSAL_PREORDER;
        } else if (strstr(query, "type=postorder")) {
            ttype = TRAVERSAL_POSTORDER;
        }

        char *json_buf = (char*)malloc(JSON_BUFFER_SIZE);
        if (!json_buf) {
            send_response(client_sock, 500, "Server Error", "application/json", "{\"error\":\"Out of memory\"}", 22);
            return;
        }

        LOCK(&g_tree_lock);
        avl_traversal_to_json(g_root, ttype, json_buf, JSON_BUFFER_SIZE);
        UNLOCK(&g_tree_lock);

        send_response(client_sock, 200, "OK", "application/json", json_buf, strlen(json_buf));
        free(json_buf);
        return;
    }

    /* POST /api/students (Register a new student) */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/students") == 0) {
        StudentData data;
        memset(&data, 0, sizeof(data));

        if (!parse_json_int(body, "student_id", &data.student_id) || data.student_id <= 0) {
            const char *err = "{\"error\":\"Valid positive Student ID is required.\"}";
            send_response(client_sock, 400, "Bad Request", "application/json", err, strlen(err));
            return;
        }

        if (!parse_json_str(body, "full_name", data.full_name, sizeof(data.full_name)) || strlen(data.full_name) == 0) {
            const char *err = "{\"error\":\"Student full name is required.\"}";
            send_response(client_sock, 400, "Bad Request", "application/json", err, strlen(err));
            return;
        }

        parse_json_str(body, "gender", data.gender, sizeof(data.gender));
        parse_json_str(body, "dob", data.dob, sizeof(data.dob));
        parse_json_str(body, "address", data.address, sizeof(data.address));
        parse_json_str(body, "contact", data.contact, sizeof(data.contact));
        parse_json_str(body, "course", data.course, sizeof(data.course));

        AVLError status = AVL_OK;
        char log_msg[256] = {0};

        LOCK(&g_tree_lock);
        g_root = avl_insert(g_root, &data, &status, log_msg, sizeof(log_msg));

        if (status == AVL_OK) {
            if (log_msg[0] != '\0') {
                strncpy(g_last_rotation_log, log_msg, sizeof(g_last_rotation_log) - 1);
                g_last_rotation_log[sizeof(g_last_rotation_log) - 1] = '\0';
            } else {
                snprintf(g_last_rotation_log, sizeof(g_last_rotation_log), "Inserted Student ID #%d (Tree remained balanced naturally)", data.student_id);
            }
            char resp[512];
            snprintf(resp, sizeof(resp),
                "{\"message\":\"Student registered successfully.\",\"student_id\":%d,\"rotation\":\"%s\"}",
                data.student_id, g_last_rotation_log);
            UNLOCK(&g_tree_lock);
            send_response(client_sock, 201, "Created", "application/json", resp, strlen(resp));
        } else if (status == AVL_ERR_DUPLICATE) {
            UNLOCK(&g_tree_lock);
            char err[256];
            snprintf(err, sizeof(err), "{\"error\":\"Duplicate Student ID #%d: A record with this ID already exists.\"}", data.student_id);
            send_response(client_sock, 409, "Conflict", "application/json", err, strlen(err));
        } else {
            UNLOCK(&g_tree_lock);
            const char *err = "{\"error\":\"Failed to insert student record into AVL tree.\"}";
            send_response(client_sock, 500, "Internal Server Error", "application/json", err, strlen(err));
        }
        return;
    }

    /* DELETE /api/students/{id} */
    if (strcmp(method, "DELETE") == 0 && strncmp(path, "/api/students/", 14) == 0) {
        int student_id = atoi(path + 14);
        if (student_id <= 0) {
            const char *err = "{\"error\":\"Invalid Student ID specified.\"}";
            send_response(client_sock, 400, "Bad Request", "application/json", err, strlen(err));
            return;
        }

        AVLError status = AVL_OK;
        char log_msg[256] = {0};

        LOCK(&g_tree_lock);
        g_root = avl_delete(g_root, student_id, &status, log_msg, sizeof(log_msg));

        if (status == AVL_OK) {
            if (log_msg[0] != '\0') {
                strncpy(g_last_rotation_log, log_msg, sizeof(g_last_rotation_log) - 1);
                g_last_rotation_log[sizeof(g_last_rotation_log) - 1] = '\0';
            } else {
                snprintf(g_last_rotation_log, sizeof(g_last_rotation_log), "Deleted Student ID #%d (Tree remained balanced)", student_id);
            }
            char resp[512];
            snprintf(resp, sizeof(resp),
                "{\"message\":\"Student record deleted successfully.\",\"student_id\":%d,\"rotation\":\"%s\"}",
                student_id, g_last_rotation_log);
            UNLOCK(&g_tree_lock);
            send_response(client_sock, 200, "OK", "application/json", resp, strlen(resp));
        } else {
            UNLOCK(&g_tree_lock);
            char err[256];
            snprintf(err, sizeof(err), "{\"error\":\"Student record with ID #%d does not exist.\"}", student_id);
            send_response(client_sock, 404, "Not Found", "application/json", err, strlen(err));
        }
        return;
    }

    /* POST /api/seed */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/seed") == 0) {
        LOCK(&g_tree_lock);
        seed_demo_students();
        UNLOCK(&g_tree_lock);

        const char *resp = "{\"message\":\"Demo student records seeded successfully into AVL tree.\"}";
        send_response(client_sock, 200, "OK", "application/json", resp, strlen(resp));
        return;
    }

    /* POST /api/clear */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/clear") == 0) {
        LOCK(&g_tree_lock);
        avl_free(g_root);
        g_root = NULL;
        strncpy(g_last_rotation_log, "Tree cleared. All records deallocated via Post-Order traversal.", sizeof(g_last_rotation_log) - 1);
        UNLOCK(&g_tree_lock);

        const char *resp = "{\"message\":\"All student records cleared from memory.\"}";
        send_response(client_sock, 200, "OK", "application/json", resp, strlen(resp));
        return;
    }

    /* Fallback: 404 */
    const char *err = "{\"error\":\"Endpoint Not Found\"}";
    send_response(client_sock, 404, "Not Found", "application/json", err, strlen(err));
}

#ifdef _WIN32
  #define NET_ERR() WSAGetLastError()
#else
  #define NET_ERR() errno
#endif

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    /* Cloud hosts (Render, Railway, Fly...) pass the port via the PORT env var */
    const char *env_port = getenv("PORT");
    if (env_port && atoi(env_port) > 0 && atoi(env_port) < 65536) port = atoi(env_port);
    if (argc > 1) {
        int custom_port = atoi(argv[1]);
        if (custom_port > 0 && custom_port < 65536) port = custom_port;
    }

    printf("===============================================================\n");
    printf(" Real-Time Student Record Management System - AVL Tree Backend \n");
    printf("===============================================================\n");

    init_base_dir();
    LOCK_INIT(&g_tree_lock);

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "[ERROR] WSAStartup failed.\n");
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN); /* don't die if a client disconnects mid-response */
#endif

    SOCKET server_sock = INVALID_SOCKET;
    bool is_ipv6 = false;

    /* Attempt dual-stack IPv6 socket first */
    server_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock != INVALID_SOCKET) {
        int v6only = 0;
        setsockopt(server_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&v6only, sizeof(v6only));

        int opt = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        struct sockaddr_in6 server_addr6;
        memset(&server_addr6, 0, sizeof(server_addr6));
        server_addr6.sin6_family = AF_INET6;
        server_addr6.sin6_addr = in6addr_any;
        server_addr6.sin6_port = htons((unsigned short)port);

        if (bind(server_sock, (struct sockaddr*)&server_addr6, sizeof(server_addr6)) == 0) {
            is_ipv6 = true;
        } else {
            closesocket(server_sock);
            server_sock = INVALID_SOCKET;
        }
    }

    /* Fallback to standard IPv4 socket if IPv6 is unavailable (common in containers) */
    if (server_sock == INVALID_SOCKET) {
        server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_sock == INVALID_SOCKET) {
            fprintf(stderr, "[ERROR] Could not create socket: %d\n", NET_ERR());
            return 1;
        }

        int opt = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        struct sockaddr_in server_addr4;
        memset(&server_addr4, 0, sizeof(server_addr4));
        server_addr4.sin_family = AF_INET;
        server_addr4.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr4.sin_port = htons((unsigned short)port);

        if (bind(server_sock, (struct sockaddr*)&server_addr4, sizeof(server_addr4)) == SOCKET_ERROR) {
            fprintf(stderr, "[ERROR] Bind failed on port %d: %d\n", port, NET_ERR());
            closesocket(server_sock);
            return 1;
        }
    }

    if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "[ERROR] Listen failed: %d\n", NET_ERR());
        closesocket(server_sock);
        return 1;
    }

    printf("[SERVER] Mode: %s\n", is_ipv6 ? "Dual-Stack (IPv6 + IPv4)" : "IPv4");
    printf("[SERVER] AVL Backend listening on port %d\n", port);
    printf("[SERVER] Dashboard: http://localhost:%d/\n", port);
    printf("---------------------------------------------------------------\n");
    fflush(stdout);

    /* Seed initial records so the dashboard opens with data */
    LOCK(&g_tree_lock);
    seed_demo_students();
    printf("[INFO] Seeded %d initial student records into balanced AVL tree.\n", avl_count(g_root));
    UNLOCK(&g_tree_lock);
    fflush(stdout);

    while (1) {
        struct sockaddr_storage client_addr;
        sock_len_t client_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);

        if (client_sock == INVALID_SOCKET) {
#ifndef _WIN32
            if (errno == EINTR) continue;
#endif
            break;
        }

        ClientContext *ctx = (ClientContext*)malloc(sizeof(ClientContext));
        if (!ctx) { closesocket(client_sock); continue; }
        ctx->sock = client_sock;

#ifdef _WIN32
        uintptr_t thread = _beginthreadex(NULL, 0, client_worker, ctx, 0, NULL);
        if (thread != 0) {
            CloseHandle((HANDLE)thread);
        } else {
            free(ctx);
            closesocket(client_sock);
        }
#else
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&tid, &attr, client_worker, ctx) != 0) {
            free(ctx);
            closesocket(client_sock);
        }
        pthread_attr_destroy(&attr);
#endif
    }

    closesocket(server_sock);
#ifdef _WIN32
    WSACleanup();
#endif
    LOCK_DESTROY(&g_tree_lock);
    avl_free(g_root);
    g_root = NULL;
    return 0;
}
