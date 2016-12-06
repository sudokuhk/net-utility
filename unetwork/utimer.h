#ifndef _UTIMER_H__
#define _UTIMER_H__

#include <unistd.h>
#include <vector>
#include <stdint.h> 

class utimermgr;

class utimer
{
public:
	utimer() 
		: abs_time_(0)
		, timer_id_(0)
		, thread_(-1)
	{
	}
	
	virtual ~utimer() {}

    int & thread() { return thread_; }

    int & waited_events() { return post_events_; }
    
private:
    int compare(const utimer & rhs)
    {
        if (abs_time_ == rhs.abs_time_) {
            return 0;
        } else if (abs_time_ > rhs.abs_time_) {
            return 1;
        } else {
            return -1;
        }
    }
    
	friend class utimermgr;

	uint64_t abs_time_;
	size_t   timer_id_;
    int      thread_;
    int      post_events_;
};

class utimermgr
{
public:
	static const uint64_t get_timestamp();

	static void sleep(const int ms);
    
public:
	utimermgr();

	~utimermgr();

	void add(int timeo, utimer * tobj);

	void remove(utimer * tobj);

	const bool empty() const;

	const int next_timeo() const;

    utimer * pop();

private:

	void heap_up(size_t);

	void heap_down(size_t);

private:
	std::vector<utimer *> heap_;
};

#endif