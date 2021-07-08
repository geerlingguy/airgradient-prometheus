# AirGradient Prometheus Exporter

[![CI](https://github.com/geerlingguy/airgradient-prometheus/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/geerlingguy/airgradient-prometheus/actions/workflows/ci.yml)

AirGradient has a [DIY air sensor](https://www.airgradient.com/diy/). I built one (actually, more than one). I want to integrate sensor data into my in-home Prometheus instance and graph the data in Grafana.

So I built this.

## How to Use

This thing is a couple PHP scripts that run in a Docker container. That's it.

You could even run the thing without using Docker, it doesn't care, it's just PHP.

### Run the Docker container

Run the PHP script inside Docker like so:

```
docker run -d -p 9925:80 --name airgradient \
  -v "$PWD":/var/www/html \
  php:8-apache \
  /bin/bash -c 'a2enmod rewrite; apache2-foreground'
```

Or you can set it up inside a docker-compose file like so:

```yaml
---
version: "3"

services:
  shelly-plug:
    container_name: airgradient
    image: php:8-apache
    ports:
      - "9925:80"
    volumes:
      - './:/var/www/html'
    restart: unless-stopped
```

### Point your AirGradient at the service

In your AirGradient's sketch, modify the `APIROOT` to point to your server, e.g.:

```ino
// change if you want to send the data to another server
String APIROOT = "http://my-own-server.net:9925/";
```

Upload the sketch to the AirGradient sensor, make sure you have it connected to your network, then you can test that the exporter has data available to it by running this `curl` command:

```
$ curl localhost:9925/metrics
# HELP instance The ID of the AirGradient sensor.
instance 1995c6
# HELP wifi Current WiFi signal strength, in dB
# TYPE wifi gauge
wifi -52
# HELP pm02 Particulat Matter PM2.5 value
# TYPE pm02 gauge
pm02 6
# HELP rc02 CO2 value, in ppm
# TYPE rc02 gauge
rco2 862
# HELP atmp Temperature, in degrees Celsius
# TYPE atmp gauge
atmp 31.6
# HELP rhum Relative humidity, in percent
# TYPE rhum gauge
rhum 38
```

If you get an error, make sure the Docker container (and the host it's on) is able to be reached over HTTP!

## License

MIT.

## Author

[Jeff Geerling](https://www.jeffgeerling.com).
