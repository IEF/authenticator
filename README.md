Authenticator
=============

Generates multiple Time-based One-Time Passwords, much like Google Authenticator.

You can change the time zone by pressing the select button.

Usage
-----

0. Start a new pebble project with ```pebble new-project authenticator``` , then clone/copy authenticator into the resulting project directory. 

1. Copy configuration-sample.txt to configuration.txt

2. Add your OTP secrets, one per line, in the format ```name:secret```

3. Generate configuration.h by running ./configuration.py

4. Build the application with 'pebble build' and copy build/authenticator.pbw to your device

The above is assuming you have the Pebble SDK installed and configured to compile watch apps.
If not, see https://developer.getpebble.com/2/getting-started/

Forked off 'twostep' by pokey9000, with patches from rigel314. Updated for SDK2.0 by wlcx
