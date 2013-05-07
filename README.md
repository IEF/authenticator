Authenticator
=============

Forked off 'authenticator' by IEF, which was forked off 'twostep' by pokey9000, this is Authenticator for Pebble,
generating multiple Time-based One-Time Passwords, much like Google Authenticator.

My changes include:
* A runtime modifiable time zone, so you don't have to recompile the app on vacation.  I wish there was a way to make this change persist.
* Making the screen immediatly change which account is displayed on the button press, rather than waiting until the next second tick.

You can change the time zone with the select button.  Then hitting the back button, will resume the app.

The same restrictions apply, you will have to recompile the application to include your
TOTP secrets, name them, and include your timezone offset from GMT.

To configure the application:
This is easily done using oathtool on linux. For example:

1. Let's say you have secret key AXIMZYJXITSIOIJVNXOE76PEJQ====== 
(you might need to pad with = characters to 16 or 32 characters, if applicable)

2. Run: oathtool --verbose --totp --base32 "AXIMZYJXITSIOIJVNXOE76PEJQ======" and copy the hex secret displayed.

3. expand the hex string 05d0cce13744e48721356ddc4ff9e44c like so: { 0x05, 0xd0, 0xcc, 0xe1, 0x37... 0x4c }

4. sed might help: echo 05d0cce13744e48721356ddc4ff9e44c|sed -re 's/(..)/0x\1, /g'

5. Enter this new expanded string into configuration.h under the otpkeys variable.

6. Label the secret under otplabels.

7. Enter the size of the secret (usually 10 for Google, 16 for others) under otpsizes.

8. Don't forget to set your time zone.

8. Build and install the application.

9. Done, you can find 'Authenticator' in your app menu for your Pebble.

The above is assuming you have the Pebble SDK installed and configured to compile watch apps.
If not, review: http://developer.getpebble.com/1/01_GetStarted/01_Step_2


