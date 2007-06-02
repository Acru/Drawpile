/*******************************************************************************

   Copyright (C) 2006, 2007 M.K.A. <wyrmchild@users.sourceforge.net>

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#ifndef ServerUser_INCLUDED
#define ServerUser_INCLUDED

#include "common.h"
#include "buffer.h" // Buffer
#include "message.h" // message_ref
#include "ev/event.h"

struct Session; // defined elsewhere
#ifndef ServerSession_INCLUDED
	#include "session.h"
#endif

#include "../shared/protocol.h"
#include "../shared/protocol.defaults.h"
#include "../shared/protocol.flags.h"

class Socket; // defined elsewhere

#ifndef NDEBUG
	#include <iostream>
#endif

#include <deque>

struct SessionData; // forward declaration

/* iterators */
#include <map>
typedef std::map<uint8_t, SessionData*>::iterator usr_session_i;
typedef std::map<uint8_t, SessionData*>::const_iterator usr_session_const_i;

typedef std::deque<message_ref>::iterator usr_message_i;
typedef std::deque<message_ref>::const_iterator usr_message_const_i;

//! User session data
struct SessionData
{
	//! ctor
	SessionData(Session &s) throw()
		: session(&s),
		layer(protocol::null_layer),
		layer_lock(protocol::null_layer),
		locked(fIsSet(s.mode, static_cast<uint8_t>(protocol::user_mode::Locked))),
		muted(fIsSet(s.mode, static_cast<uint8_t>(protocol::user_mode::Mute))),
		deaf(fIsSet(s.mode, static_cast<uint8_t>(protocol::user_mode::Deaf))),
		syncWait(false),
		cachedToolInfo(0)
	{
	}
	
	//! dtor
	~SessionData() throw() { delete cachedToolInfo; }
	
	//! Session reference
	Session *session;
	
	uint8_t
		//! Active layer
		layer,
		//! Layer to which the user is locked to
		layer_lock;
	
	bool
		//! Locked
		locked,
		//! Muted
		muted,
		//! Deaf
		deaf;
	
	//! Get user mode
	uint8_t getMode() const throw()
	{
		return (locked?(protocol::user_mode::Locked):0)
			+ (muted?(protocol::user_mode::Mute):0)
			+ (deaf?(protocol::user_mode::Deaf):0);
	}
	
	//! Set user mode
	void setMode(const uint8_t flags) throw()
	{
		locked = fIsSet(flags, static_cast<uint8_t>(protocol::user_mode::Locked));
		muted = fIsSet(flags, static_cast<uint8_t>(protocol::user_mode::Mute));
		deaf = fIsSet(flags, static_cast<uint8_t>(protocol::user_mode::Deaf));
	}
	
	//! User has sent ACK/Sync
	bool syncWait;
	
	/* cached messages */
	
	//! Cached tool info message
	protocol::ToolInfo *cachedToolInfo;
};

//! User information
struct User
	//: MemoryStack<User>
{
	//! ctor
	User(const uint8_t _id, const Socket& nsock) throw()
		: sock(nsock),
		session(0),
		id(_id),
		events(0),
		state(Init),
		layer(protocol::null_layer),
		syncing(protocol::Global),
		isAdmin(false),
		// client caps
		c_acks(false),
		// extensions
		ext_deflate(false),
		ext_chat(false),
		ext_palette(false),
		// other
		inMsg(0),
		level(0),
		deadtime(0),
		session_data(0),
		strokes(0)
	{
		#if defined(DEBUG_USER) and !defined(NDEBUG)
		std::cout << "User::User(" << static_cast<int>(_id)
			<< ", " << sock.fd() << ")" << std::endl;
		#endif
		assert(_id != protocol::null_user);
	}
	
	//! dtor
	~User() throw()
	{
		#if defined(DEBUG_USER) and !defined(NDEBUG)
		std::cout << "User::~User()" << std::endl;
		#endif
		
		delete inMsg;
		
		for (usr_session_i usi(sessions.begin()); usi != sessions.end(); ++usi)
			delete usi->second;
	}
	
	//! Change active session
	/**
	 * @param session_id The session which to activate
	 */
	bool makeActive(uint8_t session_id) throw()
	{
		session_data = getSession(session_id);
		if (session_data != 0)
		{
			session = session_data->session;
			return true;
		}
		else
			return false;
	}
	
	//! Fetch SessionData* pointer
	/**
	 * @param session_id Which session to fetch
	 */
	SessionData* getSession(uint8_t session_id) throw()
	{
		const usr_session_const_i usi(sessions.find(session_id));
		return (usi == sessions.end() ? 0 : usi->second);
	}
	
	//! Fetch const SessionData* pointer
	/**
	 * @param session_id Which session to fetch
	 */
	const SessionData* getConstSession(uint8_t session_id) const throw()
	{
		const usr_session_const_i usi(sessions.find(session_id));
		return (usi == sessions.end() ? 0 : usi->second);
	}
	
	//! Cache tool info
	/**
	 * @param ti Tool info message to cache
	 * @throw std::bad_alloc If it can't allocate local copy of the tool info
	 */
	void cacheTool(protocol::ToolInfo* ti) throw(std::bad_alloc)
	{
		assert(ti != session_data->cachedToolInfo); // attempted to re-cache same tool
		assert(ti != 0);
		
		#if defined(DEBUG_USER) and !defined(NDEBUG)
		std::cout << "Caching Tool Info for user #" << static_cast<int>(id) << std::endl;
		#endif
		
		delete session_data->cachedToolInfo;
		session_data->cachedToolInfo = new protocol::ToolInfo(*ti); // use copy-ctor
	}
	
	//! Socket
	Socket sock;
	
	//! Currently active session
	Session *session;
	
	//! User identifier
	uint id;
	
	//! Event I/O : registered events.
	// EventSystem::ev_t // inaccessible for some reason
	event_type<EventSystem>::ev_t events;
	//int events;
	
	//! User state
	enum State
	{
		//! When user has just connected
		Init,
		
		//! User has been verified to be using correct protocol.
		Verified,
		
		//! Waiting for proper user info
		Login,
		
		//! Waiting for password
		LoginAuth,
		
		//! Normal operation
		Active
	} state;
	
	uint
		//! Active layer in session
		layer,
		//! Session we're currently syncing.
		syncing;
	
	//! Is the user server admin?
	bool isAdmin;
	
	//! Client can live with ACKs alone
	bool c_acks;
	
	//! Get client capabilities
	/**
	 * @return Flags for use with the network protocol
	 */
	uint8_t getCapabilities() const throw()
	{
		return (c_acks?(protocol::client::AckFeedback):0);
	}
	
	//! Set client capabilities
	/**
	 * @param flags as used in the network protocol
	 */
	void setCapabilities(const uint8_t flags) throw()
	{
		c_acks = fIsSet(flags, static_cast<uint8_t>(protocol::client::AckFeedback));
	}
	
	bool
		//! Deflate extension
		ext_deflate,
		//! Chat extension
		ext_chat,
		//! Palette extension
		ext_palette;
	
	//! Get extensions
	/**
	 * @return Flags as used in the network protocol
	 */
	uint8_t getExtensions() const throw()
	{
		return (ext_deflate?(protocol::extensions::Deflate):0)
			+ (ext_chat?(protocol::extensions::Chat):0)
			+ (ext_palette?(protocol::extensions::Palette):0);
	}
	
	//! Set extensions
	/**
	 * @param flags as used in the network protocol
	 */
	void setExtensions(const uint8_t flags) throw()
	{
		ext_deflate = fIsSet(flags, static_cast<uint8_t>(protocol::extensions::Deflate));
		ext_chat = fIsSet(flags, static_cast<uint8_t>(protocol::extensions::Chat));
		ext_palette = fIsSet(flags, static_cast<uint8_t>(protocol::extensions::Palette));
	}
	
	//! Subscribed sessions
	std::map<uint8_t, SessionData*> sessions;
	
	//! Output queue
	std::deque<message_ref> queue;
	
	Buffer
		//! Input buffer
		input,
		//! Output buffer
		output;
	
	//! Currently incoming message.
	protocol::Message *inMsg;
	
	//! Feature level used by client
	uint level;
	
	//! Password seed associated with this user.
	char seed[4];
	
	//! Last touched.
	time_t deadtime;
	
	//! User name
	Array<char> name;
	
	//! Active session data
	SessionData* session_data;
	
	//! Stroke counter
	u_long strokes;
};

#endif // ServerUser_INCLUDED
