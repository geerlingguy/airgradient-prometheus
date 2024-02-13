# AirGradient DIY ESPHome Configuration

[ESPHome](https://esphome.io) is built for easy configuration of ESPHome-based devices, especially in tandem with home automation systems like Home Assistant.

This configuration sets up all the AirGradient hardware as sensors which can be made available to Home Assistant, and also exposes a Prometheus endpoint.

## Support Matrix

| Device | Status |
| --- | --- |
| AirGradient DIY | Supported |
| AirGradient DIY Pro | Supported |
| AirGradient ONE | See [issue #39](https://github.com/geerlingguy/airgradient-prometheus/issues/39) |

## How to Use

First, create a `secrets.yaml` file in this directory with the following contents:

```yaml
---
# Provide a unique name for this AirGradient.
name: airgradient-office

# Encryption key for HA API access.
# Generate a random 32-bit key with `openssl rand -base64 32`
api_encryption_key: PASTE_BASE64_KEY_HERE

# Set password for OTA updates.
ota_password: otapassword

# Configure a WiFi network connection.
wifi_ssid: my-wifi-network
wifi_password: my-wifi-password
```

Then, install ESPHome:

```
pip3 install esphome pillow
```

The easiest way to flash a device is to plug it in via USB, then see if it appears as a USB TTY serial device on your computer (e.g. `ls /dev`, and on my Mac, it may show up as `/dev/tty.usbserial-410` or something like that). If it doesn't appear as a serial device, you may need to do additional setup; see the [ESPHome docs](https://esphome.io/guides/physical_device_connection.html) for more details.

Once you can address the device over USB, flash the ESPHome configuration to the device:

```
esphome run airgradient-diy.yaml --device /dev/tty.usbserial-410
```

This should compile, flash, and run the configuration, and leave the logged output in your console. When you're satisfied things are working correctly, you can press Ctrl+C to exit the logs, and your AirGradient will continue running. You can now place it wherever you'd like.

If you have the [AirGradient DIY Pro kit](https://www.airgradient.com/open-airgradient/instructions/diy-pro/), use the `airgradient-diy-pro.yaml` configuration.

> You can do this setup from within the ESPHome Dashboard inside Home Assistant, but that installation method is not described here.
>
> You can also upload new configurations and debug the device over WiFi once its online, instead of passing the `/dev/tty.*` device, just pass the IP address of the AirGradient.

### Static IP address

If you would like to specify a static IP address, uncomment the relevant portion of the `wifi` configuration before running `esphome run`.

### Use with Prometheus

```sh
$ curl http://airgradient-ip-address:9926/metrics
#TYPE esphome_sensor_value GAUGE
#TYPE esphome_sensor_failed GAUGE
esphome_sensor_failed{id="temperature",name="Temperature"} 0
esphome_sensor_value{id="temperature",name="Temperature",unit="°C"} 25.9
esphome_sensor_failed{id="humidity",name="Humidity"} 0
esphome_sensor_value{id="humidity",name="Humidity",unit="%"} 33.8
esphome_sensor_failed{id="particulate_matter_10m_concentration",name="Particulate Matter <1.0µm Concentration"} 0
esphome_sensor_value{id="particulate_matter_10m_concentration",name="Particulate Matter <1.0µm Concentration",unit="µg/m³"} 3
esphome_sensor_failed{id="particulate_matter_25m_concentration",name="Particulate Matter <2.5µm Concentration"} 0
esphome_sensor_value{id="particulate_matter_25m_concentration",name="Particulate Matter <2.5µm Concentration",unit="µg/m³"} 4
esphome_sensor_failed{id="particulate_matter_100m_concentration",name="Particulate Matter <10.0µm Concentration"} 0
esphome_sensor_value{id="particulate_matter_100m_concentration",name="Particulate Matter <10.0µm Concentration",unit="µg/m³"} 6
esphome_sensor_failed{id="co2_level",name="CO2 level"} 0
esphome_sensor_value{id="co2_level",name="CO2 level",unit="ppm"} 694
#TYPE esphome_switch_value GAUGE
#TYPE esphome_switch_failed GAUGE
esphome_switch_failed{id="flash_mode_safe_mode",name="Flash Mode (Safe Mode)"} 0
esphome_switch_value{id="flash_mode_safe_mode",name="Flash Mode (Safe Mode)"} 0
```

Once you've verified it's working, configure Prometheus to scrape the sensor's endpoint: `http://airgradient-ip-address:9926/metrics`.

Example job configuration in `prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'airgradient-bedroom'
    metrics_path: /metrics
    scrape_interval: 30s
    static_configs:
      - targets: ['airgradient-ip-address:9926']
```
