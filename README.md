Authenticator
=============

Forked off 'authenticator' by IEF, which was forked off 'twostep' by pokey9000, this is Authenticator for Pebble, with patches from rigel314, updated to PebbleSDK-2.0 by Didier Arenzana.

generating multiple Time-based One-Time Passwords, much like Google Authenticator.

You can change the time zone with the select button.  Then hitting the back button, will resume the app.

To configure the application you need to create a configuration.txt file.

1. Copy configuration-sample.txt to configuration.txt

2. Set your timezone in configuration.txt - it's near the top, labelled 'tz'

3. Let's say you have secret key AXIMZYJXITSIOIJVNXOE76PEJQ 
On most sites, when setting up choose 'show key' when presented with the QR code.

4. add it to the end of configuration.txt, following the example in the format 
label:secret

5. repeat this for all your keys (don't forget to remove the example)

6. build the application by running pebble build

7. install by running pebble install

8. Done, you can find 'Authenticator' in your app menu for your Pebble.

The above is assuming you have the Pebble SDK 2.0 installed and configured to compile watch apps.
If not, review: https://developer.getpebble.com/2/
