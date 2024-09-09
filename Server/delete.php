<?php
$path = file_get_contents('php://input');
echo "GOT:" . $path;
if(!is_file($path)) {
    exit(404);
}

unlink($path);

exit(200);
?>