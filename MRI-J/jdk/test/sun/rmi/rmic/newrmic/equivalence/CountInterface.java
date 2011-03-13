/*
 * Copyright 2003 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 */

// RMI Activation Functional Test

import java.rmi.*;
import java.rmi.activation.*;

// CountInterface

public interface CountInterface extends Remote {

    public void ping() throws RemoteException;

    public int incrementCount() throws RemoteException;

    public int decrementCount() throws RemoteException;

    public int getCount() throws RemoteException;

    public int getClassCount() throws RemoteException;

    public String getProperty(String s) throws RemoteException;

    public void exit() throws RemoteException;

    // Methods specific to dealing with activatable objects

    public boolean unexportObject(boolean b) throws RemoteException;

    public ActivationID getActivationID() throws RemoteException;

    public ActivationGroupID getCurrentGroupID() throws RemoteException;

    public void inactive()
        throws RemoteException, UnknownObjectException, ActivationException;

    public void register()
        throws RemoteException, UnknownObjectException, ActivationException;

    public void unregister()
        throws RemoteException, UnknownObjectException, ActivationException;
}