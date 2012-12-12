/*
 * SharedQueue.h
 *
 *  Created on: Oct 4, 2012
 *     refer to http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
 *
 */

#ifndef SHAREDQUEUE_H_
#define SHAREDQUEUE_H_

#include <queue>
#include <boost/thread.hpp>

template <typename Data>

class SharedQueue {
private:
    std::queue<Data> the_queue;
    mutable boost::mutex the_mutex;
    boost::condition_variable the_condition_variable;

public:
    void push(Data const& data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.push(data);
        lock.unlock();
        the_condition_variable.notify_one();
    }

    void read_first_value(Data& value){

    	boost::mutex::scoped_lock lock(the_mutex);
    	if(!the_queue.empty())
    		value = the_queue.front();
    	else
    		value = NULL;
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

    bool try_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        if(the_queue.empty())
        {
            return false;
        }

        popped_value=the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        while(the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }

        popped_value=the_queue.front();
        the_queue.pop();
    }



    int size()
    {
       	return the_queue.size();
    }

    void clean(){

    	boost::mutex::scoped_lock lock(the_mutex);
    	while(!the_queue.empty()){
    		the_queue.pop();
    	}

    }


};

/*
class SharedQueue {

private:
    std::queue<Data> the_queue;
    mutable boost::mutex the_mutex;
    boost::condition_variable m_cond;

public:
    void push(const Data& data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.push(data);
        m_cond.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

    Data& front()
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.front();
    }

    Data const& front() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.front();
    }

    void pop()
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.pop();
    }

    int size()
    {
    	return the_queue.size();

    }
};
*/
#endif /* SHAREDQUEUE_H_ */
