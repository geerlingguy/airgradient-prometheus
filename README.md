# AirGradient - Prometheus WiFi Sketch

[![Arduino CI](https://github.com/geerlingguy/airgradient-prometheus/actions/workflows/arduino.yaml/badge.svg)](https://github.com/geerlingguy/airgradient-prometheus/actions/workflows/arduino.yaml)

AirGradient has a [DIY air sensor](https://www.airgradient.com/diy/). I built one (actually, more than one). I want to integrate sensor data into my in-home Prometheus instance and graph the data in Grafana.

So I built this.

## How it Works

If you're using the official AirGradient Arduino sketch (`C02_PM_SHT_OLED_WIFI`), you can configure it to enable WiFi and send data to a remote server every 9 seconds (as it cycles through the display of PM2.5, CO2, temperature, and humidity values).

By default, it sends a small JSON payload to AirGradient's servers, and you can monitor the data via their service.

This project configures the AirGradient sensor for local access (instead of delivering data to AirGradient's servers), and includes two configurations:

  1. [`AirGradient-DIY`](AirGradient-DIY/README.md): This is an Arduino sketch with all the code needed to set up an AirGradient sensor as a Prometheus endpoint on a WiFi network, suitable for scraping from any Prometheus instance (e.g. [geerlingguy/internet-pi](https://github.com/geerlingguy/internet-pi))
  2. [`AirGradient-ESPHome`](AirGradient-ESPHome/README.md): This is an ESPHome configuration which integrates the AirGradient sensor with Home Assistant using ESPHome.

Please see the README file in the respective configuration folder for more information about how to set up your AirGradient sensor.

## License

MIT.

## Authors

  - [Jeff Geerling](https://www.jeffgeerling.com)
  - [Jordan Jones](https://github.com/kashalls)

ESPHome configuration adapted from code by:

  - [Andrej Friesen](https://www.ajfriesen.com)
  - [m-reiner](https://github.com/m-reiner)
