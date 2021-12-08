# AirGradient - Prometheus WiFi Sketch

[![CI](https://github.com/geerlingguy/airgradient-prometheus/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/geerlingguy/airgradient-prometheus/actions/workflows/ci.yml)

AirGradient has a [DIY air sensor](https://www.airgradient.com/diy/). I built one (actually, more than one). I want to integrate sensor data into my in-home Prometheus instance and graph the data in Grafana.

So I built this.

## How it Works

If you're using the official AirGradient Arduino sketch (`C02_PM_SHT_OLED_WIFI`), you can configure it to enable WiFi and send data to a remote server every 9 seconds (as it cycles through the display of PM2.5, CO2, temperature, and humidity values).

By default, it sends a small JSON payload to AirGradient's servers, and you can monitor the data via their service.

This sketch provides stats upon request to the prometheus server during scrapes. By changing the deviceId in this sketch, you can easily probe multiple each airgradient from prometheus.

## How to Use

1. Point prometheus to scrape this endpoint of the airgradient sensor at: `http://your-ip:9926/metrics`.

Upload the sketch to the AirGradient sensor, make sure you have it connected to your network, then you can test that the exporter has data available to it by running this `curl` command:

```sh
$ curl http://your-ip:9926/metrics
# HELP pm02 Particulat Matter PM2.5 value
# TYPE pm02 gauge
pm02{id="Airgradient"} 6
# HELP rco2 CO2 value, in ppm
# TYPE rco2 gauge
rco2{id="Airgradient"} 862
# HELP atmp Temperature, in degrees Celsius
# TYPE atmp gauge
atmp{id="Airgradient"} 31.6
# HELP rhum Relative humidity, in percent
# TYPE rhum gauge
rhum{id="Airgradient"} 38
```

## License

MIT.

## Author

[Jeff Geerling](https://www.jeffgeerling.com).
[Jordan Jones](https://github.com/kashalls)