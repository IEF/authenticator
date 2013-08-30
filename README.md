Authenticator
=============

This app generates multiple Time-based One-Time Passwords, much like Google Authenticator.

generating multiple Time-based One-Time Passwords, much like Google Authenticator.

You can change the time zone with the select button.  Then hitting the back button, will resume the app.

To configure the application with your secrets, you need to create a configuration.txt file and compile the app.

1. Copy configuration-sample.txt to configuration.txt

2. Set your initial timezone in configuration.txt - it's near the top, labelled 'tz'

3. Let's say you have secret key AXIMZYJXITSIOIJVNXOE76PEJQ 
On most sites, when setting up choose 'show key' when presented with the QR code.

4. add it to the end of configuration.txt, following the example in the format 
label:secret

5. repeat this for all your keys (don't forget to remove the example)

6. Generate the config by running ./configuration.py

7. Link in the Pebble SDK tools from the Pebble SDK's setup script. Will be a command something like `../PebbleSDK-1.12/Pebble/tools/create_pebble_project.py --symlink-only ../PebbleSDK-1.12/Pebble/sdk .` (this was run from this authenticator's source folder), but you'll have to fiddle with the paths as appropriate depending on where you installed things.

8. Build and install the application with ./waf build, then open the `build/authenticator.pbw` file on your phone as usual. (using `python httpserver` for example, or any other web server on your computer)

9. Done! You can find 'Authenticator' in your app menu for your Pebble.

The above is assuming you have the Pebble SDK installed and configured to compile watch apps.
If not, review: http://developer.getpebble.com/1/01_GetStarted/01_Step_2

Note: Forked off 'authenticator' by IEF, which was forked off 'twostep' by pokey9000, this is Authenticator for Pebble, with patches from rigel314 
