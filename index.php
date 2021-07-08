<?php

/**
 * Handle POST requests from AirGradient sensors.
 */

// Defaults so script writes a file if parts fail.
$sensorid = 'abc';
$json_output = '';

// Handle the URL and get args.
if (!isset($_GET['path'])) {
  http_response_code(501);
  return;
}

$path = explode('/', $_GET['path']);

// Handle AirGradient requests.
if ($path[0] == 'sensors') {
  $path_part = explode(':', $path[1]);
  $instance = $path_part[1];
}

// Get the data from the POST request.
if ($data = json_decode(file_get_contents('php://input'), true)) {
  // Add instance to the array.
  $data['instance'] = $instance;
  $json_output = json_encode($data);
}
else {
  print('No data in POST request.');
  http_response_code(501);
  return;
}

// Prepare output directory.
$sensors_dir = 'sensors';
if (!is_dir($sensors_dir)) {
  mkdir($sensors_dir);
}

// Write data to file.
$output_file = $sensors_dir . '/' . $instance . '.json';
if (!file_put_contents($output_file, $json_output)) {
  print('Could not write file.');
  http_response_code(500);
  return;
}
