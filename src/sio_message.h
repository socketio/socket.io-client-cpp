//
//  sio_message.h
//
//  Created by Melo Yao on 3/25/15.
//

#ifndef __SIO_MESSAGE_H__
#define __SIO_MESSAGE_H__
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <cassert>
namespace sio
{
    using namespace std;
    
    class message
    {
    public:
        enum flag
        {
            flag_integer,
            flag_double,
            flag_string,
            flag_binary,
            flag_array,
            flag_object
        };
        
        flag get_flag() const
        {
            return _flag;
        }
        
        typedef shared_ptr<message> ptr;
        
        virtual int64_t get_int() const
        {
            assert(false);
			return 0;
        }
        
        virtual double get_double() const
        {
            assert(false);
			return 0;
        }
        
        virtual string const& get_string() const
        {
            assert(false);
			return string();
        }
        
        virtual shared_ptr<const string> const& get_binary() const
        {
            assert(false);
			return make_shared<string>();
        }
        
        virtual const vector<ptr>& get_vector() const
        {
            assert(false);
			return vector<ptr>();
        }

        virtual vector<ptr>& get_vector()
        {
            assert(false);
			return vector<ptr>();
        }
        
        virtual const map<string,message::ptr>& get_map() const
        {
            assert(false);
			return map<string,message::ptr>();
        }
        
        virtual map<string,message::ptr>& get_map()
        {
            assert(false);
			return map<string,message::ptr>();
        }
    private:
        flag _flag;
        
    protected:
        message(flag f):_flag(f){}
    };
    
    
    class int_message : public message
    {
        int64_t _v;
    protected:
        int_message(int64_t v)
        :message(flag_integer),_v(v)
        {
        }
        
    public:
        static message::ptr create(int64_t v)
        {
            return ptr(new int_message(v));
        }
        
        int64_t get_int() const
        {
            return _v;
        }
    };
    
    class double_message : public message
    {
        double _v;
        double_message(double v)
        :message(flag_double),_v(v)
        {
        }
        
    public:
        static message::ptr create(double v)
        {
            return ptr(new double_message(v));
        }
        
        double get_double() const
        {
            return _v;
        }
    };
    
    class string_message : public message
    {
        string _v;
        string_message(string const& v)
        :message(flag_string),_v(v)
        {
        }
    public:
        static message::ptr create(string const& v)
        {
            return ptr(new string_message(v));
        }
        
        string const& get_string() const
        {
            return _v;
        }
    };
    
    class binary_message : public message
    {
        shared_ptr<const string> _v;
        binary_message(shared_ptr<const string> const& v)
        :message(flag_binary),_v(v)
        {
        }
    public:
        static message::ptr create(shared_ptr<const string> const& v)
        {
            return ptr(new binary_message(v));
        }
        
        shared_ptr<const string> const& get_binary() const
        {
            return _v;
        }
    };
    
    class array_message : public message
    {
        vector<message::ptr> _v;
        array_message():message(flag_array)
        {
        }
        
    public:
        static message::ptr create()
        {
            return ptr(new array_message());
        }
        
        vector<ptr>& get_vector()
        {
            return _v;
        }
        
        const vector<ptr>& get_vector() const
        {
            return _v;
        }
    };
    
    class object_message : public message
    {
        map<string,message::ptr> _v;
        object_message() : message(flag_object)
        {
        }
    public:
        static message::ptr create()
        {
            return ptr(new object_message());
        }
        
        map<string,message::ptr>& get_map()
        {
            return _v;
        }
        
        const map<string,message::ptr>& get_map() const
        {
            return _v;
        }
    };
    
    inline
    message::ptr make_message(string const& event_name, message::ptr const& event_args)
    {
        message::ptr msg_ptr = array_message::create();
        array_message* ptr = static_cast<array_message*>(msg_ptr.get());
        ptr->get_vector().push_back(string_message::create(event_name));
        if(event_args)
        {
            ptr->get_vector().push_back(event_args);
        }
        return msg_ptr;
    }
    
    inline
    message::ptr make_message(string const& event_name, string const& single_message)
    {
        message::ptr msg_ptr = array_message::create();
        array_message* ptr = static_cast<array_message*>(msg_ptr.get());
        ptr->get_vector().push_back(string_message::create(event_name));
        ptr->get_vector().push_back(string_message::create(single_message));
        return msg_ptr;
    }
    
    inline
    message::ptr make_message(string const& event_name, shared_ptr<const string> const& single_binary)
    {
        message::ptr msg_ptr = array_message::create();
        array_message* ptr = static_cast<array_message*>(msg_ptr.get());
        ptr->get_vector().push_back(string_message::create(event_name));
        if(single_binary)
        {
            ptr->get_vector().push_back(binary_message::create(single_binary));
        }
        return msg_ptr;
    }
}

#endif
