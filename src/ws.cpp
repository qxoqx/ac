#include <ws.h>
#include <iostream>
#include <spdlog/spdlog.h>

int wsConn::connected = 0;
int wsConn::done = 0;


void* process_proc(void* p_server)
{
    auto conn = (wsConn*)p_server;

    while(!wsConn::done)
    {
        mg_mgr_poll(conn->GetMgr(), 100);
    }
    return NULL;
}

void wsConn::connect(const std::string &url) {
    this->mgr = new(mg_mgr);
    mg_connection* nc;

    mg_mgr_init(this->mgr, NULL);
    nc = mg_connect_ws(this->mgr, wsConn::ev_handler, url.c_str(), nullptr, nullptr);

    if (nc == NULL) {
        spdlog::error("Invalid address");
        return;
    }
    this->nc = nc;

    mg_start_thread(process_proc, this);


//    while(!wsConn::done) {
//        mg_mgr_poll(this->mgr, 100);
//    }

    spdlog::debug("wsConn connect done");

}

bool wsConn::isConnected() {
    return this->connected;
}


void wsConn::ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    (void) nc;

//    std::cout << "handle" << std::endl;

    switch (ev) {
        case MG_EV_CONNECT: {
            int status = *((int *) ev_data);
            if (status != 0) {
                spdlog::error("ws connect error: {}", status);
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
            auto *hm = (struct http_message *) ev_data;
            if (hm->resp_code == 101) {
                spdlog::info("ws connected");
                wsConn::connected = 1;

                char msg[1024];
                strcpy(msg, "connected");
                mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, msg, strlen(msg));

            } else {
                spdlog::error("ws connected failed, http code: {}", hm->resp_code);
                /* Connection will be closed after this. */
            }
            break;
        }
        case MG_EV_POLL: {
//            char msg[500];
//            int n = 0;
//
//            fd_set read_set, write_set, err_set;
//            struct timeval timeout = {0, 0};
//            FD_ZERO(&read_set);
//            FD_ZERO(&write_set);
//            FD_ZERO(&err_set);
//            FD_SET(0 /* stdin */, &read_set);
//            if (select(1, &read_set, &write_set, &err_set, &timeout) == 1) {
//                n = read(0, msg, sizeof(msg));
//            }
//            if (n <= 0) break;
//            while (msg[n - 1] == '\r' || msg[n - 1] == '\n') n--;
//            mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, msg, n);
            break;
        }
        case MG_EV_WEBSOCKET_FRAME: {
            struct websocket_message *wm = (struct websocket_message *) ev_data;
            spdlog::info("{}", wm->data);
            break;
        }
        case MG_EV_CLOSE: {
            if (wsConn::connected)
                spdlog::info("ws disconnected");
            wsConn::connected = false;
            break;
        }
    }
}

bool wsConn::send(const std::string &wsMessage) {
    if(!wsConn::connected) {
        std::cout << "not connected" << std::endl;
        return false;
    }
    char msg[204800];
    strcpy(msg, wsMessage.c_str());
    mg_send_websocket_frame(this->nc, WEBSOCKET_OP_TEXT, msg, strlen(msg));
    return true;
}

mg_mgr *wsConn::GetMgr() {
    return this->mgr;
}

void wsConn::close() {
    wsConn::done = true;
    mg_mgr_free(this->mgr);
}
