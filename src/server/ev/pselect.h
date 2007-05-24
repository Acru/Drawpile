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

#ifndef EventPselect_INCLUDED
#define EventPselect_INCLUDED

#include "../common.h"
#include "interface.h"

#ifndef NDEBUG
	#include <iostream>
	using std::cout;
	using std::endl;
#endif

#include <sys/select.h>
#include <signal.h>

#include <map>
#include <set>

class EvPselect
	: EvInterface<int>
{
private:
	timespec _timeout;
	int nfds;
	sigset_t sigmask, sigsaved;
	fd_set fds_r, fds_w, fds_e, t_fds_r, t_fds_w, t_fds_e;
	std::map<int, uint> fd_list; // events set for FD
	std::map<int, uint>::iterator fd_iter;
	std::set<int> read_set, write_set, error_set;
	int nfds_r, nfds_w, nfds_e;
public:
	static const int
		read,
		write,
		error;

	EvPselect() throw()
	{
	}
	
	~EvPselect() throw()
	{
	}
	
	//! Set signal mask.
	/**
	 * Only used by pselect() so far
	 */
	void setMask(const sigset_t& mask) throw() { sigmask = mask; }
	
	void timeout(uint msecs) throw()
	{
		#ifndef NDEBUG
		std::cout << "pselect.timeout(msecs: " << msecs << ")" << std::endl;
		#endif
		
		if (msecs > 1000)
		{
			timeout.tv_sec = msecs/1000;
			msecs -= _timeout.tv_sec*1000;
		}
		else
			_timeout.tv_sec = 0;
		
		_timeout.tv_nsec = msecs * 1000000; // nanoseconds
	}
	
	int wait() throw()
	{
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "pselect.wait()" << endl;
		#endif
		
		#ifdef EV_SELECT_COPY
		FD_COPY(&fds_r, &t_fds_r),
		FD_COPY(&fds_w, &t_fds_w),
		FD_COPY(&fds_e, &t_fds_e);
		#else
		t_fds_r = fds_r;
		t_fds_w = fds_w;
		t_fds_e = fds_e;
		#endif // HAVE_SELECT_COPY
		
		// save sigmask
		sigprocmask(SIG_SETMASK, &_sigmask, &_sigsaved);
		
		const fd_t largest_nfds = std::max(std::max(nfds_w, nfds_r), nfds_e);
		
		nfds = pselect((largest_nfds+1), &t_fds_r, &t_fds_w, &t_fds_e, &_timeout, &_sigmask);
		
		_error = errno;
		sigprocmask(SIG_SETMASK, &_sigsaved, 0); // restore mask
	
	switch (nfds)
	{
		case -1:
			if (_error == EINTR)
				return nfds = 0;
			
			assert(_error != EBADF);
			assert(_error != ENOTSOCK);
			assert(_error != EINVAL);
			assert(_error != EFAULT);
			
			break;
		case 0:
			// do nothing
			break;
		default:
			fd_iter = fd_list.begin();
			break;
	}
	
	return nfds;
	}
	
	int add(fd_t fd, int events) throw()
	{
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "pselect.add(fd: " << fd << ")" << endl;
		#endif
		
		assert(fd != INVALID_SOCKET);
		
		bool rc=false;
		
		if (fIsSet(events, read))
		{
			FD_SET(fd, &fds_r);
			read_set.insert(read_set.end(), fd);
			nfds_r = *(read_set.end());
			rc = true;
		}
		if (fIsSet(events, write))
		{
			FD_SET(fd, &fds_w);
			write_set.insert(write_set.end(), fd);
			nfds_w = *(--write_set.end());
			rc = true;
		}
		if (fIsSet(events, error))
		{
			FD_SET(fd, &fds_e);
			error_set.insert(error_set.end(), fd);
			nfds_e = *(--error_set.end());
			rc = true;
		}
		
		// maintain fd_list
		fd_list[fd] = events;
		
		return rc;
	}
	
	int remove(fd_t fd) throw()
	{
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "pselect.remove(fd: " << fd << ")" << endl;
		#endif
		
		assert(fd != INVALID_SOCKET);
		
		std::map<fd_t,uint>::iterator iter(fd_list.find(fd));
		if (iter == fd_list.end())
			return false;
		
		FD_CLR(fd, &fds_r);
		read_set.erase(fd);
		nfds_r = (read_set.size() > 0 ? *(--read_set.end()) : 0);
		
		FD_CLR(fd, &fds_w);
		write_set.erase(fd);
		nfds_w = (write_set.size() > 0 ? *(--write_set.end()) : 0);
		
		FD_CLR(fd, &fds_e);
		error_set.erase(fd);
		nfds_e = (error_set.size() > 0 ? *(--error_set.end()) : 0);
		
		fd_list.erase(iter);
		fd_iter = fd_list.begin();
		
		return true;
	}
	
	int modify(fd_t fd, int events) throw()
	{
		#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
		cout << "pselect.modify(fd: " << fd << ")" << endl;
		#endif
		
		assert(fd != INVALID_SOCKET);
		
		// act like a wrapper.
		if (events != 0)
			add(fd, events);
		
		if (!fIsSet(events, read))
		{
			FD_CLR(fd, &fds_r);
			read_set.erase(fd);
			nfds_r = (read_set.size() > 0 ? *(--read_set.end()) : 0);
		}
		
		if (!fIsSet(events, write))
		{
			FD_CLR(fd, &fds_w);
			write_set.erase(fd);
			nfds_w = (write_set.size() > 0 ? *(--write_set.end()) : 0);
		}
		
		if (!fIsSet(events, error))
		{
			FD_CLR(fd, &fds_e);
			error_set.erase(fd);
			nfds_e = (error_set.size() > 0 ? *(--error_set.end()) : 0);
		}
		
		return 0;
	}
	
	bool getEvent(fd_t &fd, int &events) throw()
	{
		while (fd_iter != fd_list.end())
		{
			fd = fd_iter->first;
			++fd_iter;
			
			events = 0;
			
			if (FD_ISSET(fd, &t_fds_r) != 0)
				fSet(events, read);
			if (FD_ISSET(fd, &t_fds_w) != 0)
				fSet(events, write);
			if (FD_ISSET(fd, &t_fds_e) != 0)
				fSet(events, error);
			
			if (events != 0)
				return true;
		}
		
		return false;
	}
};

#include "traits.h"

template <>
struct EvTraits<EvPselect>
{
	typedef int ev_t;
	
	static inline
	bool hasHangup() { return false; }
	
	static inline
	bool hasError() { return true; }
	
	static inline
	bool hasAccept() { return false; }
	
	static inline
	bool hasConnect() { return false; }
	
	static inline
	bool usesSigmask() { return true; }
};

#endif // EventPselect_INCLUDED
