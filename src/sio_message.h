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
#include <type_traits>
namespace sio
{
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
            flag_object,
            flag_boolean,
            flag_null
        };

        virtual ~message(){};

        class list;

        flag get_flag() const
        {
            return _flag;
        }

        typedef std::shared_ptr<message> ptr;

        virtual bool get_bool() const
        {
            assert(false);
            return false;
        }

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

        virtual std::string const& get_string() const
        {
            assert(false);
            static std::string s_empty_string;
            s_empty_string.clear();
            return s_empty_string;
        }

        virtual std::shared_ptr<const std::string> const& get_binary() const
        {
            assert(false);
            static std::shared_ptr<const std::string> s_empty_binary;
            s_empty_binary = nullptr;
            return s_empty_binary;
        }

        virtual const std::vector<ptr>& get_vector() const
        {
            assert(false);
            static std::vector<ptr> s_empty_vector;
            s_empty_vector.clear();
            return s_empty_vector;
        }

        virtual std::vector<ptr>& get_vector()
        {
            assert(false);
            static std::vector<ptr> s_empty_vector;
            s_empty_vector.clear();
            return s_empty_vector;
        }

        virtual const std::map<std::string,message::ptr>& get_map() const
        {
            assert(false);
            static std::map<std::string,message::ptr> s_empty_map;
            s_empty_map.clear();
            return s_empty_map;
        }

        virtual std::map<std::string,message::ptr>& get_map()
        {
            assert(false);
            static std::map<std::string,message::ptr> s_empty_map;
            s_empty_map.clear();
            return s_empty_map;
        }
    private:
        flag _flag;

    protected:
        message(flag f):_flag(f){}
    };

    class null_message : public message
    {
    protected:
        null_message()
            :message(flag_null)
        {
        }

    public:
        static message::ptr create()
        {
            return ptr(new null_message());
        }
    };

    class bool_message : public message
    {
        bool _v;

    protected:
        bool_message(bool v)
            :message(flag_boolean),_v(v)
        {
        }

    public:
        static message::ptr create(bool v)
        {
            return ptr(new bool_message(v));
        }

        bool get_bool() const
        {
            return _v;
        }
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

        double get_double() const//add double accessor for integer.
        {
            return static_cast<double>(_v);
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
        std::string _v;
        string_message(std::string const& v)
            :message(flag_string),_v(v)
        {
        }

        string_message(std::string&& v)
            :message(flag_string),_v(move(v))
        {
        }
    public:
        static message::ptr create(std::string const& v)
        {
            return ptr(new string_message(v));
        }

        static message::ptr create(std::string&& v)
        {
            return ptr(new string_message(move(v)));
        }

        std::string const& get_string() const
        {
            return _v;
        }
    };

    class binary_message : public message
    {
        std::shared_ptr<const std::string> _v;
        binary_message(std::shared_ptr<const std::string> const& v)
            :message(flag_binary),_v(v)
        {
        }
    public:
        static message::ptr create(std::shared_ptr<const std::string> const& v)
        {
            return ptr(new binary_message(v));
        }

        std::shared_ptr<const std::string> const& get_binary() const
        {
            return _v;
        }
    };

    class array_message : public message
    {
        std::vector<message::ptr> _v;
        array_message():message(flag_array)
        {
        }

    public:
        static message::ptr create()
        {
            return ptr(new array_message());
        }

        void push(message::ptr const& message)
        {
            if(message)
                _v.push_back(message);
        }

        void push(const std::string& text)
        {
            _v.push_back(string_message::create(text));
        }

        void push(std::string&& text)
        {
            _v.push_back(string_message::create(move(text)));
        }

        void push(std::shared_ptr<std::string> const& binary)
        {
            if(binary)
                _v.push_back(binary_message::create(binary));
        }

        void push(std::shared_ptr<const std::string> const& binary)
        {
            if(binary)
                _v.push_back(binary_message::create(binary));
        }

        void insert(size_t pos,message::ptr const& message)
        {
            _v.insert(_v.begin()+pos, message);
        }

        void insert(size_t pos,const std::string& text)
        {
            _v.insert(_v.begin()+pos, string_message::create(text));
        }

        void insert(size_t pos,std::string&& text)
        {
            _v.insert(_v.begin()+pos, string_message::create(move(text)));
        }

        void insert(size_t pos,std::shared_ptr<std::string> const& binary)
        {
            if(binary)
                _v.insert(_v.begin()+pos, binary_message::create(binary));
        }

        void insert(size_t pos,std::shared_ptr<const std::string> const& binary)
        {
            if(binary)
                _v.insert(_v.begin()+pos, binary_message::create(binary));
        }

        size_t size() const
        {
            return _v.size();
        }

        const message::ptr& at(size_t i) const
        {
            return _v[i];
        }

        const message::ptr& operator[] (size_t i) const
        {
            return _v[i];
        }

        std::vector<ptr>& get_vector()
        {
            return _v;
        }

        const std::vector<ptr>& get_vector() const
        {
            return _v;
        }
    };

    class object_message : public message
    {
        std::map<std::string,message::ptr> _v;
        object_message() : message(flag_object)
        {
        }
    public:
        static message::ptr create()
        {
            return ptr(new object_message());
        }

        void insert(const std::string & key,message::ptr const& message)
        {
            _v[key] = message;
        }

        void insert(const std::string & key,const std::string& text)
        {
            _v[key] = string_message::create(text);
        }

        void insert(const std::string & key,std::string&& text)
        {
            _v[key] = string_message::create(move(text));
        }

        void insert(const std::string & key,std::shared_ptr<std::string> const& binary)
        {
            if(binary)
                _v[key] = binary_message::create(binary);
        }

        void insert(const std::string & key,std::shared_ptr<const std::string> const& binary)
        {
            if(binary)
                _v[key] = binary_message::create(binary);
        }

        bool has(const std::string & key)
        {
            return _v.find(key) != _v.end();
        }

        const message::ptr& at(const std::string & key) const
        {
            static std::shared_ptr<message> not_found;

            std::map<std::string,message::ptr>::const_iterator it = _v.find(key);
            if (it != _v.cend()) return it->second;
            return not_found;
        }

        const message::ptr& operator[] (const std::string & key) const
        {
            return at(key);
        }

        bool has(const std::string & key) const
        {
            return _v.find(key) != _v.end();
        }

        std::map<std::string,message::ptr>& get_map()
        {
            return _v;
        }

        const std::map<std::string,message::ptr>& get_map() const
        {
            return _v;
        }
    };

    class message::list
    {
    public:
        list()
        {
        }

        list(std::nullptr_t)
        {
        }

        list(message::list&& rhs):
            m_vector(std::move(rhs.m_vector))
        {

        }

        list & operator= (const message::list && rhs)
        {
            m_vector = std::move(rhs.m_vector);
            return *this;
        }

        template <typename T>
        list(T&& content,
            typename std::enable_if<std::is_same<std::vector<message::ptr>,typename std::remove_reference<T>::type>::value>::type* = 0):
            m_vector(std::forward<T>(content))
        {
        }

        list(message::list const& rhs):
            m_vector(rhs.m_vector)
        {

        }

        list(message::ptr const& message)
        {
            if(message)
                m_vector.push_back(message);

        }

        list(const std::string& text)
        {
            m_vector.push_back(string_message::create(text));
        }

        list(std::string&& text)
        {
            m_vector.push_back(string_message::create(move(text)));
        }

        list(std::shared_ptr<std::string> const& binary)
        {
            if(binary)
                m_vector.push_back(binary_message::create(binary));
        }

        list(std::shared_ptr<const std::string> const& binary)
        {
            if(binary)
                m_vector.push_back(binary_message::create(binary));
        }

        void push(message::ptr const& message)
        {
            if(message)
                m_vector.push_back(message);
        }

        void push(const std::string& text)
        {
            m_vector.push_back(string_message::create(text));
        }

        void push(std::string&& text)
        {
            m_vector.push_back(string_message::create(move(text)));
        }

        void push(std::shared_ptr<std::string> const& binary)
        {
            if(binary)
                m_vector.push_back(binary_message::create(binary));
        }

        void push(std::shared_ptr<const std::string> const& binary)
        {
            if(binary)
                m_vector.push_back(binary_message::create(binary));
        }

        void insert(size_t pos,message::ptr const& message)
        {
            m_vector.insert(m_vector.begin()+pos, message);
        }

        void insert(size_t pos,const std::string& text)
        {
            m_vector.insert(m_vector.begin()+pos, string_message::create(text));
        }

        void insert(size_t pos,std::string&& text)
        {
            m_vector.insert(m_vector.begin()+pos, string_message::create(move(text)));
        }

        void insert(size_t pos,std::shared_ptr<std::string> const& binary)
        {
            if(binary)
                m_vector.insert(m_vector.begin()+pos, binary_message::create(binary));
        }

        void insert(size_t pos,std::shared_ptr<const std::string> const& binary)
        {
            if(binary)
                m_vector.insert(m_vector.begin()+pos, binary_message::create(binary));
        }

        size_t size() const
        {
            return m_vector.size();
        }

        const message::ptr& at(size_t i) const
        {
            return m_vector[i];
        }

        const message::ptr& operator[] (size_t i) const
        {
            return m_vector[i];
        }

        message::ptr to_array_message(std::string const& event_name) const
        {
            message::ptr arr = array_message::create();
            arr->get_vector().push_back(string_message::create(event_name));
            arr->get_vector().insert(arr->get_vector().end(),m_vector.begin(),m_vector.end());
            return arr;
        }

        message::ptr to_array_message() const
        {
            message::ptr arr = array_message::create();
            arr->get_vector().insert(arr->get_vector().end(),m_vector.begin(),m_vector.end());
            return arr;
        }

    private:
        std::vector<message::ptr> m_vector;
    };
}

#endif
