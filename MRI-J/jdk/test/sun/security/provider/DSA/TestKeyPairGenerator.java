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

/*
 * @test
 * @bug 4800108
 * @summary verify that precomputed DSA parameters are always used (512, 768, 1024 bit)
 * @run main/othervm/timeout=15 TestKeyPairGenerator
 */

// this fix is really a performance fix, so this test is not foolproof
// without it, it will take a minute or more (unless you have a very fast machine)
// with the fix, the test should complete in <2 seconds
// use 15 second timeout to leave some room

import java.security.*;
import java.security.interfaces.*;

public class TestKeyPairGenerator {

    private static void checkKeyLength(KeyPair kp, int len) throws Exception {
        DSAPublicKey key = (DSAPublicKey)kp.getPublic();
        int n = key.getParams().getP().bitLength();
        System.out.println("Key length: " + n);
        if (len != n) {
            throw new Exception("Wrong key length");
        }
    }

    public static void main(String[] args) throws Exception {
        long start = System.currentTimeMillis();
        KeyPairGenerator kpg;
        KeyPair kp;
        // problem was when not calling initialize()
        // do that twice to artifically inflate the time
        // on JDKs that do not have the fix
        kpg = KeyPairGenerator.getInstance("DSA", "SUN");
        kp = kpg.generateKeyPair();
        checkKeyLength(kp, 1024);

        kpg = KeyPairGenerator.getInstance("DSA", "SUN");
        kp = kpg.generateKeyPair();
        checkKeyLength(kp, 1024);

        // some other basic tests
        kp = kpg.generateKeyPair();
        checkKeyLength(kp, 1024);

        kpg.initialize(1024);
        kp = kpg.generateKeyPair();
        checkKeyLength(kp, 1024);

        kpg.initialize(768);
        kp = kpg.generateKeyPair();
        checkKeyLength(kp, 768);

        kpg.initialize(512);
        kp = kpg.generateKeyPair();
        checkKeyLength(kp, 512);

        long stop = System.currentTimeMillis();
        System.out.println("Time: " + (stop - start) + " ms.");
    }

}
