# WUnderground-2-PWS-Weather-Bridge
Takes Weather Underground from your PWS and Updated PWS Weather every 5 mins

Once you have flashed you ESP01 or any other ESP8266 device and give it power, you will need to connect to the soft AP SSID of the device.
it will be in the format 
SSID: WU2PWSWBridge1M_<last 4 chars of MAC address>
Password: 123456789

Once connected to this SSID, you will then need to open up a web browser and navigate to http://192.168.4.1/
After the page loads you should then connect to your WiFi network and then it should connect to the internet.

Once it is connected to your WiFi, you can then access the main page either by IP or by the host name http://WU2PWSWBridge1M_XXXX.local/
where XXXX is, it will be your unique last 4 digits/chars of the device MAC address.

Things you will need:
Weather Underground API (https://www.wunderground.com/member/api-keys)
This will allow you to access your PWS data that you have pushed to WU
The name of your PWS as it is on WU under devices

PWSWeather.com :
You will need to create a station, and then in software enter "WU2PWSWBridge2.5"
you will need to copy the station ID and the API KEY.

once all that is setup you can then enter this informatio0n on the config pages in the browser. 
If everything checks out, you should start to see data on the serial monitor and the main web page as it refreshes every 15 seconds.

Please note!
*** Data from new devieces on PWS Weather will say "BAD DATA" for about a week or less so it can validate your data against other stations near by.
Also if using this for Rachio Weather Intelegence, it will sometimes take 2 weeks before your station will appear in the Rachio APP to select from.
