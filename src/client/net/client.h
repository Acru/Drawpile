/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2013-2014 Calle Laakkonen

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
#ifndef DP_NET_CLIENT_H
#define DP_NET_CLIENT_H

#include <QObject>

#include "core/point.h"
#include "../shared/net/message.h"

namespace paintcore {
	class Point;
}

namespace drawingboard {
	class ToolContext;
}

namespace protocol {
	class SnapshotMode;
	class Chat;
	class UserJoin;
	class UserAttr;
	class UserLeave;
	class SessionConf;
	class LayerACL;
	class MovePointer;
	class Marker;
}

namespace net {
	
class Server;
class LoopbackServer;
class LoginHandler;
class UserListModel;
class LayerListModel;

/**
 * The client for accessing the drawing server.
 */
class Client : public QObject {
Q_OBJECT
public:
	Client(QObject *parent=0);
	~Client();

	/**
	 * @brief Connect to a remote server
	 * @param loginhandler the login handler to use
	 */
	void connectToServer(LoginHandler *loginhandler);

	/**
	 * @brief Disconnect from the remote server
	 */
	void disconnectFromServer();

	/**
	 * @brief Pause input message handling
	 * @param pause
	 */
	void pauseInput(bool pause);

	/**
	 * @brief Get the local user's user/context ID
	 * @return user ID
	 */
	int myId() const { return _my_id; }

	/**
	 * @brief Get the local user's username
	 * @return user name
	 */
	QString myName() const;

	/**
	 * @brief Is the client connected to a local server?
	 *
	 * A local server is one that is running on this computer
	 * and thus has minimum latency.
	 * @return true if server is local
	 */
	bool isLocalServer() const;

	/**
	 * @brief Is the client connected by network?
	 * @return true if a network connection is open
	 */
	bool isConnected() const { return !_isloopback; }

	/**
	 * @brief Is the user connected and logged in?
	 * @return true if there is an active network connection and login process has completed
	 */
	bool isLoggedIn() const;

	/**
	 * @brief Is there a global lock on?
	 *
	 * @return true if there is a session or user lock
	 */
	bool isLocked() const { return _isSessionLocked || _isUserLocked; }

	/**
	 * @brief Is the currently logged in user a session operator?
	 *
	 * This is always true in local mode.
	 * @return true
	 */
	bool isOperator() const { return _isloopback || _isOp; }

	/**
	 * @brief Get the number of bytes waiting to be sent
	 * @return upload queue length
	 */
	int uploadQueueBytes() const;

	/**
	 * @brief Get the user list
	 * @return user list model
	 */
	UserListModel *userlist() const { return _userlist; }

	/**
	 * @brief Get the layer list
	 * @return layer list model
	 */
	LayerListModel *layerlist() const { return _layerlist; }

	//! Reinitialize after clearing out the old board
	void init();

public slots:
	// Layer changing
	void sendCanvasResize(int top, int right, int bottom, int left);
	void sendNewLayer(int id, const QColor &fill, const QString &title);
	void sendLayerAttribs(int id, float opacity, int blend);
	void sendLayerTitle(int id, const QString &title);
	void sendLayerVisibility(int id, bool hide);
	void sendLayerReorder(const QList<uint8_t> &ids);
	void sendDeleteLayer(int id, bool merge);

	// Drawing
	void sendToolChange(const drawingboard::ToolContext &ctx);
	void sendStroke(const paintcore::Point &point);
	void sendStroke(const paintcore::PointVector &points);
	void sendPenup();
	void sendImage(int layer, int x, int y, const QImage &image, bool blend);
	void sendFillRect(int layer, const QRect &rect, const QColor &color, int blend=255);

	// Undo/redo
	void sendUndopoint();
	void sendUndo(int actions=1, int override=0);
	void sendRedo(int actions=1, int override=0);

	// Annotations
	void sendAnnotationCreate(int id, const QRect &rect);
	void sendAnnotationReshape(int id, const QRect &rect);
	void sendAnnotationEdit(int id, const QColor &bg, const QString &text);
	void sendAnnotationDelete(int id);

	// Snapshot	
	void sendLocalInit(const QList<protocol::MessagePtr> commands);
	void sendSnapshot(const QList<protocol::MessagePtr> commands);

	// Misc.
	void sendChat(const QString &message);
	void sendLaserPointer(const QPointF &point, int trail=0);
	void sendMarker(const QString &text);

	// Operator commands
	void sendLockUser(int userid, bool lock);
	void sendKickUser(int userid);
	void sendOpUser(int userid, bool op);
	void sendSetSessionTitle(const QString &title);
	void sendLayerAcl(int layerid, bool locked, QList<uint8_t> exclusive);
	void sendLockSession(bool lock);
	void sendLockLayerControls(bool lock);
	void sendCloseSession(bool close);

	// Recording
	void playbackCommand(protocol::MessagePtr msg);
	void endPlayback();

signals:
	void messageReceived(protocol::MessagePtr msg);
	void drawingCommandReceived(protocol::MessagePtr msg);
	void chatMessageReceived(const QString &user, const QString &message, bool me);
	void markerMessageReceived(const QString &user, const QString &message);
	void needSnapshot(bool forcenew);
	void userPointerMoved(int ctx, const QPointF &point, int trail);

	void serverConnected(const QString &address, int port);
	void serverLoggedin(bool join);
	void serverDisconnecting();
	void serverDisconnected(const QString &message);

	void userJoined(int id, const QString &name);
	void userLeft(const QString &name);

	void canvasLocked(bool locked);
	void opPrivilegeChange(bool op);
	void sessionTitleChange(const QString &title);
	void sessionConfChange(bool locked, bool layerctrllocked, bool closed);
	void lockBitsChanged();

	void layerVisibilityChange(int id, bool hidden);

	void expectingBytes(int);
	void sendingBytes(int);
	void bytesReceived(int);
	void bytesSent(int);

private slots:
	void handleMessage(protocol::MessagePtr msg);
	void handleConnect(int userid, bool join);
	void handleDisconnect(const QString &message);

private:
	void handleSnapshotRequest(const protocol::SnapshotMode &msg);
	void handleChatMessage(const protocol::Chat &msg);
	void handleMarkerMessage(const protocol::Marker &msg);
	void handleUserJoin(const protocol::UserJoin &msg);
	void handleUserAttr(const protocol::UserAttr &msg);
	void handleUserLeave(const protocol::UserLeave &msg);
	void handleSessionConfChange(const protocol::SessionConf &msg);
	void handleLayerAcl(const protocol::LayerACL &msg);
	void handleMovePointer(const protocol::MovePointer &msg);

	Server *_server;
	LoopbackServer *_loopback;
	
	int _my_id;
	bool _isloopback;
	bool _isOp;
	bool _isSessionLocked, _isUserLocked;
	UserListModel *_userlist;
	LayerListModel *_layerlist;
};

}

#endif
