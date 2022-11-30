#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <uv.h>

#include <functional>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Raise
{
public:

    Raise(const char * error)
    {
        printf("error: %s\n",error);
        assert(false);
   
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Call
{
public:

	class Function
	{
	};


	// Static functions dont have an instance, but the function pointer we are
	// going to use will keep one as a placeholder so that the static function
	// can use the same storage as a member function.

	class Base
	{
	};


	// Im not making any assumptions that the size of a member pointer in a class
	// with multiple inheritance is the same as the size of a member pointer in a
	// class with virtual inheritance.  Though according to many sources they are.
	class Derived : public Base, public Function
	{
	};

	class Virtual : virtual Derived
	{
	};


	class Unknown;

	typedef int (*CallFunction)(int);
	typedef int (Base::*CallBase)(int);
	typedef int (Derived::*CallDerived)(int);
	typedef int (Virtual::*CallVirtual)(int);
	typedef int (Unknown::*CallUnknown)(int);

	static const int SizeofFunction = sizeof(CallFunction);
	static const int SizeofBase = sizeof(CallBase);
	static const int SizeofDerived = sizeof(CallDerived);
	static const int SizeofVirtual = sizeof(CallVirtual);
	static const int SizeofUnknown = sizeof(CallUnknown);

	// The types are just the sizes so they can be identified easily, but in order to
	// call them we need to atleast differentiate between a function pointer and a 
	// member function pointer.  So we offset all of the member function pointers.
	enum CallType
	{
		TYPE_FUNCTION	= 1,
		TYPE_BASE		= 2,
		TYPE_DERIVED	= 3,
		TYPE_VIRTUAL	= 4,
		TYPE_UNKNOWN	= 5,
        TYPE_FUNCTOR    = 6,
	};


    // The same signature as a normal static function since we use a helper
    typedef int (*CallFunctor)(void *, int);
    
	// The largest type should always be the unknown pointer, so by setting that to
	// null in the constructor the entire union should be null.
	union Calls
	{
        CallFunctor Functor;    
		CallFunction Function;
		CallBase Base;
		CallDerived Derived;
		CallVirtual Virtual;
		CallUnknown Unknown;

		Calls():Unknown(0) {}
	};

public:

	short Type;
	short Args;
	void * That;
	Calls Func;

	Call():
		Type(0),Args(0),That(0)
	{
	}

	Call(const Call & call):
		Type(call.Type),Args(call.Args),That(call.That),Func(call.Func)
	{
	}

	Call(short type, short args, void * that):
        Type(type),Args(args),That(that)
	{
	}

	operator bool () 
	{
		return Type != 0 && Func.Unknown != 0;
	}


    struct Counted
    {
        int References;

        Counted():References(0) {}
        virtual ~Counted() {}
        
        void Increment()
        {
            ++References;
        }

        void Decrement()
        {
            --References;
            if (References <= 0)
                delete this;
        }
    };

    template<typename _Functor_,typename _Return_, typename... _Args_>
    struct Functor : Counted
    {
    public:

        _Functor_ Func;

        Functor(_Functor_ && func):
            Func(func) 
        {

        }
        
        _Return_ operator () (_Args_... args)
        {
            return Func(args...);
        }

    };

};

template <typename _Return_, typename... _Args_>
class Callback : public Call
{
public:

	#define Sizeofcall(call) (sizeof(call)<=SizeofBase?2:(sizeof(call)<=SizeofDerived?3:(sizeof(call)<=SizeofVirtual?4:(sizeof(call)<=SizeofUnknown?5:0))))


	Callback()
	{
	}

    ~Callback()
    {   
        if (Type == TYPE_FUNCTOR)
        {
            ((Call::Counted*)That)->Decrement();
        }
    }

    Callback(const Callback & callback):Call((Call&)callback)
    {        
        if (Type == TYPE_FUNCTOR)
        {
            ((Call::Counted*)That)->Increment();
        }
    }

    Callback & operator = (const Callback & callback)
    {
        Call::operator = ((Call&)callback);
        if (Type == TYPE_FUNCTOR)
        {
            ((Call::Counted*)That)->Increment();
        }

        return *this;
    }

    template<typename _Functor_>
    Callback(_Functor_ && func):Call(6,-1,0)
    {
        typedef Functor<_Functor_,_Return_,_Args_&&...> FunctorType;
        struct FunctorInvoke
        {            
            static _Return_ Invoke(void * that,_Args_&&...args)
            {   
                ((FunctorType*)that)->operator()((_Args_&&)args...);
            }
        };

        That = (void*)new FunctorType((_Functor_&&)func);
        
        // GCC is fine with this simple form, Clang requires the memcpy indirection
        //Func.Functor = &FunctorInvoke::Invoke;    
        struct Info {_Return_ (*func)(void*,_Args_&&...);} info = {&FunctorInvoke::Invoke};
        memcpy(&Func,&info,sizeof(info));

        Type = Call::TYPE_FUNCTOR;

        ((Call::Counted*)That)->Increment();

    }

	template<typename _Class_>
	Callback(_Return_ (_Class_::*call)(_Args_...)):Call(Sizeofcall(call),-1,0)
	{
		struct Info {_Return_ (_Class_::*func)(_Args_...);} info = {call};        
		memcpy(&Func,&info,sizeof(info));
	}

	template<typename _Class_>
	Callback(_Class_ * that, _Return_ (_Class_::*call)(_Args_...)):Call(Sizeofcall(call),-1,that)
	{
		struct Info {_Return_ (_Class_::*func)(_Args_...);} info = {call};
		memcpy(&Func,&info,sizeof(info));
	}

	Callback(_Return_ (*call)(_Args_...)):Call(1,-1,0)
	{
		struct Info {_Return_ (*func)(_Args_...);} info = {call};
		memcpy(&Func,&info,sizeof(info));
	}

    template<typename..._Arg_>
    _Return_ operator() (_Args_...args)
	{
		switch(Type)
		{
        case TYPE_FUNCTOR: typedef _Return_ (*FunctorInvoke)(void *, _Args_...); return ((FunctorInvoke)Func.Functor)(That,(_Args_)args...);
        case TYPE_FUNCTION: typedef _Return_ (*FunctionPrototype)(_Args_...); return ((FunctionPrototype)Func.Function)((_Args_)args...);
		case TYPE_BASE: typedef _Return_ (Base::*BasePrototype)(_Args_...); return (((Base*)That)->*((BasePrototype)Func.Base))((_Args_)args...);
		case TYPE_DERIVED: typedef _Return_ (Derived::*DerivedPrototype)(_Args_...); return (((Derived*)That)->*((DerivedPrototype)Func.Derived))((_Args_)args...);
		case TYPE_VIRTUAL: typedef _Return_ (Virtual::*VirtualPrototype)(_Args_...); return (((Virtual*)That)->*((VirtualPrototype)Func.Virtual))((_Args_)args...);
		default: typedef _Return_ (Unknown::*UnknownPrototype)(_Args_...); return (((Unknown*)That)->*((UnknownPrototype)Func.Unknown))((_Args_)args...);
		}
	}	

    
    template<typename _Class_>
    _Return_ operator() (_Class_ * that, _Args_... args) const
	{
		switch(Type)
		{
		case TYPE_BASE: typedef _Return_ (Base::*BasePrototype)(_Args_...); return (((Base*)that)->*((BasePrototype)Func.Base))((_Args_)args...);        
		case TYPE_DERIVED: typedef _Return_ (Derived::*DerivedPrototype)(_Args_...); return (((Derived*)that)->*((DerivedPrototype)Func.Derived))((_Args_)args...);
		case TYPE_VIRTUAL: typedef _Return_ (Virtual::*VirtualPrototype)(_Args_...); return (((Virtual*)that)->*((VirtualPrototype)Func.Virtual))((_Args_)args...);
		default: typedef _Return_ (Unknown::*UnknownPrototype)(_Args_...); return (((Unknown*)that)->*((UnknownPrototype)Func.Unknown))((_Args_)args...);
        }
	}

};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class ArraySet
{
public:

    void * Data[256];
    int Size;


    ArraySet()
    {
        memset(Data,0,256*sizeof(void*));
        Size=0;
    }

    virtual int Compare(void * lhs, void * rhs)
    {
        return (int*)lhs-(int*)rhs;
    }

    int Index(void * handle)
    {
        int first	= 0;    
		int last	= this->Size-1;
		int middle	= first + (int)((last-first+1)>>1);
		int result	= 0;

		while (first <= last)
		{
			result = Compare(handle,Data[middle]);
			if (result == 0) break;
			
			if (result < 0)
				last = middle-1;
			else
				first = middle+1;

			// Update the middle at the end so it always gets set to the index where
			// the compare should have been, even if we exit the loop.				
			middle = first + (int)((last-first+1)>>1);				
		}	
			
		return middle;
	}
	
    void Insert(void *handle)
    {
        int index = Index(handle);                
        Data[index] = handle;
        ++Size;

        index = Index(handle);
    }

    void * Select(void * handle)
    {
        int index = Index(handle);
        return Data[index];
    }

    void * Remove(void * handle)
    {
        // The handle may be a pointer to an object which has just
        // been used to implement Compare in a derived class and may
        // not be the same as the handle stored at index.
        int index = Index(handle);
        handle = Data[index];
        
        Data[index] = 0;
        --Size;

        return handle;
    }

    
};

class ArrayMap : ArraySet
{
public:
    
    struct Mapped
    {
        void * Key;
        void * Value;
        Mapped(void * key=0,void * value=0):Key(key),Value(value) {}
    };

    int Compare(void * lhs, void * rhs)
    {
        Mapped * ml = (Mapped*)lhs;
        Mapped * mr = (Mapped*)rhs;

        return (int*)ml->Key - (int*)mr->Key;
    }

    void Insert(void * key, void * value)
    {   
        Mapped * m = new Mapped(key,value);
        ArraySet::Insert(m);
    }

    void * Select(void * key)
    {
        Mapped m(key);
        Mapped * mr = (Mapped*)ArraySet::Select(&m);
        return mr->Value;
    }

    void * Remove(void * key)
    {
        Mapped m(key);
        Mapped * mr = (Mapped*)ArraySet::Remove(&m);  
        
        void * value = mr->Value;      
        delete mr;
        
        return value;

    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class EventLoop
{
public:

    uv_loop_t Handle;

    EventLoop():Handle()
    {
        uv_loop_init(&Handle);
    }

    ~EventLoop()
    {
        uv_loop_close(&Handle);
    }

    void Run()
    {
        uv_run(&Handle,UV_RUN_DEFAULT);
    }

    void RunOnce()
    {
        uv_run(&Handle,UV_RUN_ONCE);
    }

};




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Timer
{
public:

    Callback<void,void*> Event;

    EventLoop * Loop;
    uv_timer_t Handle;

    Timer(EventLoop * loop):Loop(loop)
    {
        if (Loop)
        {
            uv_timer_init(&Loop->Handle,&Handle);
            Handle.data = this;
        }
    }

    static void Handler(uv_timer_t * handle)
    {
        Timer * timer = (Timer*)handle->data;
        if (timer->Event)
            timer->Event(timer);
    }

    void Start(int timeout, int repeat)
    {
        uv_timer_start(&Handle,&Handler,timeout,repeat);        
    }

    void Stop()
    {
        uv_timer_stop(&Handle);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Idler 
{
public:

    Callback<void,void*> Event;

    EventLoop * Loop;
    uv_idle_t Handle;


    Idler(EventLoop * loop):Loop(loop)
    {
        if (Loop)
        {
            uv_idle_init(&Loop->Handle,&Handle);
            Handle.data = this;
        }
    }

    static void Handler(uv_idle_t * handle)
    {
        Idler * timer = (Idler*)handle->data;
        if (timer->Event)
            timer->Event(timer);        
    }

    void Start()
    {
        uv_idle_start(&Handle,&Handler);        
    }

    void Stop()
    {
        uv_idle_stop(&Handle);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct Address : sockaddr
{        
    Address()
    {
        memset(((sockaddr*)this),0,sizeof(sockaddr));
    }

    Address(char * host, int port, int family = AF_INET)
    {            
        // Create an inet address by default
        ((sockaddr_in*)this)->sin_family = family;
        ((sockaddr_in*)this)->sin_port = 0;
        ((sockaddr_in*)this)->sin_addr.s_addr = 0; 

        // Nice but not necessary, zero the rest of the struct 
        memset(&(((sockaddr_in*)this)->sin_zero),0,8);

        Host(host);
        Port(port);
    }

    int Family()
    {
        return ((sockaddr*)this)->sa_family;
    }

    char * Ip()
    {
        return inet_ntoa(((sockaddr_in*)this)->sin_addr);
    }
                
    int Port()
    {
        return ntohs(((sockaddr_in*)this)->sin_port);
    }                    

    void Port(int port)
    {
        ((sockaddr_in*)this)->sin_port = htons(port);
    }

    unsigned long Host()
    {
        return ((sockaddr_in*)this)->sin_addr.s_addr;
    }

    void Host(char * host)
    {
        // Allow resolving either hostname or dotted decimal ip
        Host(inet_addr(host));
        if (Host() == INADDR_NONE)
        {
            // Resolve ip address
            unsigned long ip=0;
            hostent *entry = gethostbyname(host);
            if (entry)
                ip = ((in_addr *)entry->h_addr_list[0])->s_addr;

            Host(ip);
            if(Host() == INADDR_NONE || Host() == 0)
            {
                Raise("Could not resolve hostname or ip");
            }
        }
    }

    void Host(unsigned long host)
    {
        ((sockaddr_in*)this)->sin_addr.s_addr = host;
    }        
};




class TcpSocket
{
public:

    // Array of handles for connect/shutdown and other socket events
    ArrayMap Handles;

    Callback<void,void*,int> ConnectEvent;
    Callback<void,void*> ShutdownEvent;
    Callback<void,void*> CloseEvent;
    Callback<void,void*, int> AcceptEvent;
    Callback<void,void*, int> WriteEvent;
    Callback<void,void*, char*, int> ReadEvent;

    EventLoop * Loop;
    uv_tcp_t Handle;

    
    static void ConnectHandler(uv_connect_t * handle, int status)
    {   
        // The connection handler is given a new connect handle which we
        // created when making the connection, and need to delete here.
        TcpSocket * socket = (TcpSocket*)handle->data;
        if (socket->ConnectEvent)
            socket->ConnectEvent(socket,status);

        delete handle;
    }

    static void AcceptHandler(uv_stream_t * handle, int status)
    {
        // The stream is just the uv_tcp_t as accept is one of the common
        // functions used between sockets/pipes etc
        TcpSocket * socket = (TcpSocket*)handle->data;
        if (socket->AcceptEvent)
            socket->AcceptEvent(socket,status);
    }
    
    static void WriteHandler(uv_write_t * handle, int status)
    {
        TcpSocket * socket = (TcpSocket*)handle->data;
        if (socket->WriteEvent)
            socket->WriteEvent(socket,status);

        uv_buf_t * buf = (uv_buf_t*)socket->Handles.Remove(handle);
        delete buf;
    }

    static void AllocHandler(uv_handle_t * handle, unsigned long size, uv_buf_t * buf)
    {
        buf->base = new char[size];
        buf->len = size;
    }
    
    static void CloseHandler(uv_handle_t* handle)
    {
        TcpSocket * socket = (TcpSocket*)handle->data;
        if (socket->CloseEvent)
            socket->CloseEvent(socket);        
    }

    static void ReadHandler(uv_stream_t * handle, signed long size, const uv_buf_t * buf)
    {
        if (size == -1) 
        {
            Raise("Read error!\n");
            // Ignore the close callback
            uv_close((uv_handle_t*)handle, &CloseHandler);
            return;
        }

        if (size == 0)
        {
            // Retry/Read again
        }
        else
        if (size == UV_EOF)
        {
            // Eof
        }

        TcpSocket * socket = (TcpSocket*)handle->data;
        if (socket->ReadEvent)
            socket->ReadEvent(socket,buf->base,size);        

        delete[] buf->base;
    }


    TcpSocket(EventLoop * loop):Loop(loop)
    {
        if (Loop)
        {
            uv_tcp_init(&Loop->Handle,&Handle);
            Handle.data = this;
        }
    }

    void Addr(Address & addr)
    {
        int length=0;
        //uv_tcp_getsockname(&Handle,&addr,&length);
        uv_tcp_getpeername(&Handle,&addr,&length);
    }

    void Bind(const char * host, int port, int flags=0)
    {
        Address addr((char*)host,port);
        uv_tcp_bind(&Handle,&addr,flags);
    }

    void Connect(const char * host, int port)
    {
        Address addr((char*)host,port);

        // We can delete the handle in the callback
        uv_connect_t * handle = new uv_connect_t();
        handle->data = this;

        uv_tcp_connect(handle,&Handle,&addr,&ConnectHandler);    
    }

    void Listen(int backlog=10)
    {        
        uv_listen((uv_stream_t*)&Handle,backlog,&AcceptHandler);
    }

    void Accept(TcpSocket * server)
    {
        uv_accept((uv_stream_t*)&server->Handle,(uv_stream_t*)&Handle);
    }


    void Write(const char * data)
    {
        int size =  (data==0)?0:strlen(data);
        Write((char*)data,size);
    }

    void Write(char * data, int size)
    {
        uv_write_t * handle = new uv_write_t();
        
        uv_buf_t * buf = new uv_buf_t();
        buf->base = data;
        // Recall that strlen() will segfault if data is 0
        buf->len = size;
        
        // If we want to be able to delete the buf on the write callback we
        // will need to map it or create a new struct to hold more than one
        // pointer.  That might be generally a good idea, but incurs more 
        // allocation and makes the interface a little more confusing for
        // the user.
        handle->data = this;

        // Add the write handle so we can delete it on callback
        Handles.Insert(handle,buf);

        uv_write(handle,(uv_stream_t*)&Handle,buf,1,&WriteHandler);
    }

    void Read()
    {
        uv_read_start((uv_stream_t*)&Handle,&AllocHandler,&ReadHandler);

    }
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{

    // Test the event loop and our simple TCP echo server
    
    // Connent using netcat i.e. nc localhost 1234 and type
    // anything to have it repeated back to you.

    static bool done=false;
    static EventLoop loop;

    struct Helper
    {

        static void OnStatic(void * arg)
        {
            printf("static: %p\n",arg);
        }

        void OnMember(void * arg)
        {
            printf("this: %p member: %p\n",this,arg);
        }

        void operator () (void * arg)
        {
            printf("this: %p functor: %p\n",this,arg);
        }

        static void OnTimer(void * arg)
        {
            printf("timer: %p\n",arg);
            

            /*
            Timer timer(&loop);
            timer.Start(10,0);
            timer.Event = &Helper::OnTimerNested;

            // We just turned an async call into a sync call        
            while (!done)
                loop.RunOnce();

            // This is like a Win32 modal dialog
            */

        }

        static void OnTimerNested(void * arg)
        {
            done=true;
        }

        static void OnIdle(void * arg)
        {
            printf("idle: %p\n",arg);
            
        }



    };

    Helper helper;


    {
        // Test the callback template

        // Run the callback with a pointer to itself
        Callback<void,void*> ca;
        Callback<void,void*> cb;
        void * thing = &cb;
        void * thang = &ca;
        printf("thing: %p\n",thing);
        printf("thang: %p\n",thang);
        
        // Static function
        cb = &Helper::OnStatic;        
        cb(thing);
        ca = cb;
        ca(thing);

        // Functor
        cb = helper;
        cb(thing);
        ca = cb;
        ca(thing);

        // Member function with this
        cb = Callback<void,void*>(&helper,&Helper::OnMember);
        cb(thing);
        ca = cb;
        ca(thing);

        // Member function with delayed this
        cb = &Helper::OnMember;
        cb(&helper,thing);
        ca = cb;
        // Can be called without valid this, provided it doesnt access any local 
        // variables it wont crash.
        ca(thing);        
        ca(&helper,thing);

    }
    
    Timer timer(&loop);
    timer.Start(200,200);
    // Now bind the event
    //timer.Event = &Helper::OnTimer;

    Idler idle(&loop);
    // If we don't bind the idle event it will still happen but our callback
    // wont be called at the last stage, very easy to add/remove.
    //idle.Event = &Helper::OnIdle;
    idle.Start();

    struct TcpServer
    {
        // Can't use clients unless its static, so now we need a callback that 
        // can support member functions
        ArraySet Clients;

        void OnAccept(void * arg, int status)
        {            
            printf("this: %p\n",this);

            TcpSocket * server = (TcpSocket*)arg;
            
            TcpSocket * client = new TcpSocket(server->Loop);
            client->Accept(server);

            Address addr;
            client->Addr(addr);

            char buf[256];
            snprintf(buf,256,"Hello %s %d\n",addr.Ip(),addr.Port());
            
            // GCC can deduce this without template args, but Clang can't

            //client->WriteEvent = Callback(this,&TcpServer::OnWrite);
            client->WriteEvent = Callback<void,void*,int>(this,&TcpServer::OnWrite);                            
            client->Write(buf);

            //client->ReadEvent = Callback(this,&TcpServer::OnRead);
            client->ReadEvent = Callback<void,void*,char*,int>(this,&TcpServer::OnRead);
            client->Read();  

            Clients.Insert(client);
        }

        void OnWrite(void * arg, int status)
        {
            // We write to the client socket
            TcpSocket * client = (TcpSocket*)arg;
            printf("write: %p\n",arg);
        }

        void OnRead(void * arg, char * data, int size)
        {
            printf("read: %.*s",size,data);
            
            // Echo server
            TcpSocket * client = (TcpSocket*)arg;
            client->Write(data,size);

        }
    };  


    TcpServer server;
    
    TcpSocket socket(&loop);

    //socket.AcceptEvent = Callback(&server,&TcpServer::OnAccept);
    socket.AcceptEvent = Callback<void,void*,int>(&server,&TcpServer::OnAccept);

    // Bind to all ports
    socket.Bind("0.0.0.0",1234);
    socket.Listen();


    printf("running...\n");

    loop.Run();

    printf("sleeping...\n");
    sleep(200);        
    


}