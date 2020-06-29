#include <mongoose.h>
#include <string>
#include <mutex>


#ifndef AC_WS_H
#define AC_WS_H
#endif //AC_WS_H

class wsConn {
public:
    static wsConn* get_instance()
    {
        static wsConn instance;
        return &instance;
    }

    static int connected;
    static int done;

    void connect(const std::string &url);
    bool isConnected();

    bool send(const std::string&);
    void close();

    static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);

    mg_mgr* GetMgr();

private:
    wsConn(){};

    static wsConn *p_instance;  //用类的指针指向唯一的实例

    class GC
    {
    public:
        ~GC()
        {
            if (p_instance != NULL)
            {
                delete p_instance;
                p_instance = NULL;
            }
        }
    };
    static GC gc;

    mg_connection* nc = nullptr;
    mg_mgr *mgr = nullptr;

};