<?php
define ('SITE_ROOT', realpath(dirname(__FILE__)));

if(!isset($_FILES["uploadFile"])) {
    die("no file provided");
}

$extension = strtolower(pathinfo($_FILES["uploadFile"]["name"], PATHINFO_EXTENSION));
$filename = date('Y-m-d_H-i-s', time()) . "." . $extension;


$success = move_uploaded_file($_FILES["uploadFile"]["tmp_name"], SITE_ROOT . "/uploads/" . $filename);
if($success) {
    http_response_code(200);
    echo "The file ". basename( $_FILES["uploadFile"]["name"]). " has been uploaded.";
    file_put_contents("log.txt", date('Y-m-d_H-i-s', time()) . ": SUCCESS " . SITE_ROOT . "/uploads/" . $filename . "\r\n", FILE_APPEND);
}
else {
    http_response_code(422);
    echo "Sorry, there was an error uploading your file: '" . $_FILES["uploadFile"]["error"] . "'";
    file_put_contents("errors.txt", date('Y-m-d_H-i-s', time()) . ": " . $_FILES["uploadFile"]["error"] . "\r\n", FILE_APPEND);
}
header ("Connection: close");
