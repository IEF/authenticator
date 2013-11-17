Authenticator
=============

Generates multiple Time-based One-Time Passwords, much like Google Authenticator.

You can change the time zone with the select button.  Then hitting the back button, will resume the app.

To configure the application you need to create a configuration.txt file.

1. Copy configuration-sample.txt to configuration.txt

2. Let's say you have secret key AXIMZYJXITSIOIJVNXOE76PEJQ 
On most sites, when setting up choose 'show key' when presented with the QR code.

3. add it to the end of configuration.txt, following the example in the format 
label:secret

4. repeat this for all your keys (don't forget to remove the example)

5. Generate the config by running ./configuration.py

6. Build and install the application with ./waf build && python httpserver as usual

7. Done, you can find 'Authenticator' in your app menu for your Pebble.

8. Set your timezone in the app

The above is assuming you have the Pebble SDK installed and configured to compile watch apps.
If not, review: http://developer.getpebble.com/1/01_GetStarted/01_Step_2

Forked off 'twostep' by pokey9000, with patches from rigel314. Updated for SDK2.0 by wlcx