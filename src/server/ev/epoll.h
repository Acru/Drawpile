/*******************************************************************************

   Copyright (C) 2007 M.K.A. <wyrmchild@users.sourceforge.net>
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
   
   Except as contained in this notice, the name(s) of the above copyright
   holders shall not be used in advertising or otherwise to promote the sale,
   use or other dealings in this Software without prior written authorization.
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#ifndef EventEpoll_INCLUDED
#define EventEpoll_INCLUDED

#include "../common.h"
#include "interface.h"

#ifndef NDEBUG
	#include <iostream>
	using std::cout;
	using std::endl;
#endif
#include <sys/epoll.h>

template <int max_events>
class EvEpoll
	: EvInterface<int>
{
private:
	uint _timeout;
	int nfds;
	int evfd;
	epoll_event events[max_events];
public:
	static const int
		read,
		write,
		error;
	
	EvEpoll() throw(std::exception)
	{
		evfd = epoll_create(max_events);
		
		if (evfd == -1)
		{
			_error = errno;
			
			// max_events is not positive integer
			assert(_error != EINVAL);
			
			throw std::exception;
		}
	}
	
	~EvEpoll() throw()
	{
		if (evfd != -1)
		{
			close(evfd);
			evfd = -1;
		}
		
		// Make sure the event fd was closed.
		assert(evfd == -1);
	}
	
	void timeout(uint msecs) throw()
	{
		#ifndef NDEBUG
		cout << "epoll.timeout(msecs: " << msecs << ")" << endl;
		#endif
		
		_timeout = msecs;
	}
	
	int wait() throw()
	{
		assert(evfd != 0);
		
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "epoll.wait()" << endl;
		#endif
		
		nfds = epoll_wait(evfd, events, max_events, _timeout);
		
		if (nfds == -1)
		{
			_error = errno;
			
			if (_error == EINTR)
				return 0;
			
			assert(_error != EBADF);
			assert(_error != EFAULT); // events not writable
			assert(_error != EINVAL); // invalif evfd, or max_events <= 0
		}
		
		return nfds;
	}
	
	int add(fd_t fd, int events) throw()
	{
		assert(evfd != 0);
		
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "epoll.add(FD: " << fd << ")" << endl;
		#endif
		
		assert(fd != INVALID_SOCKET);
		
		epoll_event ev_info;
		ev_info.data.fd = fd;
		ev_info.events = events;
		
		const int r = epoll_ctl(evfd, EPOLL_CTL_ADD, fd, &ev_info);
		
		if (r == -1)
		{
			_error = errno;
			
			assert(_error != EBADF);
			assert(_error != EINVAL); // epoll fd is invalid, or fd is same as epoll fd
			assert(_error != EEXIST); // fd already in set
			assert(_error != EPERM); // target fd not supported by epoll
			
			return false;
		}
		
		return true;
	}
	
	int remove(fd_t fd) throw()
	{
		assert(evfd != 0);
		
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "epoll.remove(FD: " << fd << ")" << endl;
		#endif
		
		assert(fd != INVALID_SOCKET);
		
		const int r = epoll_ctl(evfd, EPOLL_CTL_DEL, fd, 0);
		
		if (r == -1)
		{
			_error = errno;
			
			assert(_error != EBADF); // evfd is invalid
			assert(_error != EINVAL); // evfd is invalid, or evfd is the same as fd
			assert(_error != ENOENT); // fd not in set
			
			return false;
		}
		
		return true;
	}
	
	int modify(fd_t fd, int events) throw()
	{
		assert(evfd != 0);
		
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "epoll.modify(FD: " << fd << ")" << endl;
		#endif
		
		assert(fd != INVALID_SOCKET);
		
		epoll_event ev_info;
		ev_info.data.fd = fd;
		ev_info.events = events;
		
		const int r = epoll_ctl(evfd, EPOLL_CTL_MOD, fd, &ev_info);
		
		if (r == -1)
		{
			_error = errno;
			
			assert(_error != EBADF); // epoll fd is invalid
			assert(_error != EINVAL); // evfd is invalid or fd is the same as evfd
			assert(_error != ENOENT); // fd not in set
			
			return false;
		}
		
		return true;
	}
	
	bool getEvent(fd_t &fd, int &events) throw()
	{
		assert(evfd != 0);
		
		if (nfds == -1)
			return false;
		
		fd = events[nfds].data.fd;
		r_events = events[nfds].events;
		--nfds;
		
		return true;
	}
};

#include "traits.h"

template <>
struct EventTraits<EvEpoll>
{
	typedef int ev_t;
	
	static inline
	bool hasHangup() { return true; }
	
	static inline
	bool hasError() { return true; }
	
	static inline
	bool hasAccept() { return false; }
	
	static inline
	bool hasConnect() { return false; }
	
	static inline
	bool usesSigmask() { return false; }
};

#endif // EventEpoll_INCLUDED
