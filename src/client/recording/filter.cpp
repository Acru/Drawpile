/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2014 Calle Laakkonen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <QDebug>
#include <QVector>
#include <QHash>
#include <QSet>

#include "../shared/record/reader.h"
#include "../shared/record/writer.h"
#include "../shared/net/undo.h"

#include "filter.h"

namespace recording {

namespace {

static const uchar UNDOABLE = (1<<7);
static const uchar REMOVED = (1<<6);

struct FilterIndex {
	uchar type;
	uchar ctxid;
	uchar flags;
};

struct State {
	QVector<FilterIndex> index;
	QHash<int, QList<int>> userjoins;
	QSet<int> users_seen;
};

void mark_delete(FilterIndex &i) {
	i.flags |= REMOVED;
}

inline protocol::MessageUndoState undostate(const FilterIndex &i) {
	return protocol::MessageUndoState(i.flags & 0x03);
}

inline bool isUndoable(const FilterIndex &i) {
	return i.flags & UNDOABLE;
}

bool isDeleted(const FilterIndex &i) {
	return i.flags & (REMOVED | protocol::UNDONE);
}

void filterMessage(const Filter &filter, State &state, protocol::MessagePtr msg)
{
	// Put this message in the index
	{
		FilterIndex imsg;
		imsg.type = msg->type();
		imsg.ctxid = msg->contextId();
		imsg.flags = msg->isUndoable() ? UNDOABLE : 0 | msg->undoState();
		state.index.append(imsg);
	}

	// Filter out select message types
	switch(msg->type()) {
	using namespace protocol;
	case MSG_CHAT:
		if(filter.removeChat()) {
			mark_delete(state.index.last());
			return;
		}
		break;

	case MSG_USER_JOIN:
	case MSG_USER_ATTR:
	case MSG_USER_LEAVE:
		state.userjoins[msg->contextId()].append(state.index.size()-1);
		return;

	case MSG_INTERVAL:
		if(filter.removeDelays()) {
			mark_delete(state.index.last());
			return;
		}

	case MSG_MOVEPOINTER:
		if(filter.removeLasers()) {
			mark_delete(state.index.last());
			return;
		}

	case MSG_MARKER:
		if(filter.removeMarkers()) {
			mark_delete(state.index.last());
			return;
		}

	default: break;
	}

	// Perform undo
	if(msg->type() == protocol::MSG_UNDO && filter.expungeUndos()) {
		// Perform an undo. This is a stripped down version of handleUndo from StateTracker
		const protocol::Undo &cmd = msg.cast<protocol::Undo>();

		const uchar ctxid = cmd.contextId();
		const bool undo = cmd.points()>0;
		int actions = qAbs(cmd.points());

		// Step 1. Find undo or redo point
		int pos = state.index.size();
		if(undo) {
			// Search for undoable actions from the end of the
			// command stream towards the beginning
			while(actions>0 && --pos>=0) {
				const FilterIndex u = state.index[pos];

				if(u.type == protocol::MSG_UNDOPOINT && u.ctxid == ctxid) {
					if(undostate(u) == protocol::DONE)
						--actions;
				}
			}
		} else {
			// Find the start of the undo sequence
			int redostart = pos;
			while(--pos>=0) {
				const FilterIndex u = state.index[pos];
				if(u.type == protocol::MSG_UNDOPOINT && u.ctxid == ctxid) {
					if(undostate(u) != protocol::DONE)
						redostart = pos;
					else
						break;
				}
			}

			if(redostart == state.index.size()) {
				qWarning() << "nothing to redo for user" << cmd.contextId();
				mark_delete(state.index.last());
				return;
			}
			pos = redostart;
		}

		// Step 2 is not needed here
		// Step 3. (Un)mark all actions by the user as undone
		if(undo) {
			for(int i=pos;i<state.index.size();++i) {
				FilterIndex &u = state.index[i];
				if(u.ctxid == ctxid && isUndoable(u))
					u.flags |= protocol::UNDONE;
			}
		} else {
			int i=pos;
			++actions;
			while(i<state.index.size()) {
				FilterIndex &u = state.index[i];
				if(u.ctxid == ctxid) {
					if(u.type == protocol::MSG_UNDOPOINT && undostate(u) != protocol::GONE)
						if(--actions==0)
							break;

					// GONE messages cannot be redone
					if(undostate(u) == protocol::UNDONE)
						u.flags &= UNDOABLE;
				}
				++i;
			}
		}

		// Steps 4 & 5 not needed here.
	}

	if(msg->contextId()>0)
		state.users_seen.insert(msg->contextId());

	return;
}

//! Remove all users who haven't done anything
void filterLookyloos(State &state)
{
	foreach(int id, state.userjoins.keys()) {
		if(!state.users_seen.contains(id)) {
			foreach(int idx, state.userjoins[id]) {
				mark_delete(state.index[idx]);
			}
		}
	}
}

void doFilterRecording(Filter &filter, State &state, Reader &recording)
{
	while(true) {
		MessageRecord msg = recording.readNext();
		if(msg.status == MessageRecord::END_OF_RECORDING)
			break;
		if(msg.status == MessageRecord::INVALID) {
			qWarning() << "skipping invalid message type" << msg.type;
			continue;
		}

		protocol::MessagePtr msgp(msg.message);
		filterMessage(filter, state, msgp);
	}

	if(filter.removeLookyloos())
		filterLookyloos(state);
}

}

Filter::Filter()
	: _expunge_undos(false), _remove_chat(false), _remove_lookyloos(false), _remove_delays(false),
	  _remove_lasers(false), _remove_markers(false)
{
}

bool Filter::filterRecording(QFileDevice *input, QFileDevice *output)
{
	// Step 1. Open input and output files
	Reader reader(input);
	Compatibility readOk = reader.open();
	if(readOk != COMPATIBLE && readOk != MINOR_INCOMPATIBILITY) {
		qWarning() << "Cannot open recording for filtering. Error code" << readOk;
		_errormsg = reader.errorString();
		return false;
	}

	Writer writer(output);
	if(!writer.open()) {
		qWarning() << "Cannot open record filter output file";
		_errormsg = writer.errorString();
		return false;
	}

	// Step 2. Mark messages for removal
	State state;
	doFilterRecording(*this, state, reader);

	// Step 4. Write messages back to output file
	reader.rewind();

	writer.writeHeader();

	unsigned int index=0;
	QByteArray buffer;
	while(reader.readNextToBuffer(buffer)) {
		if(!isDeleted(state.index[index]))
			writer.writeFromBuffer(buffer);

		++index;
	}

	return true;
}

}
