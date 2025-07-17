<?php
if ($_POST) {
    // CHECK IF THE REQUEST IS FROM OUR DEVICE
    if (!isset($_POST["cihaz"])) {
        header("Location: http://your-domain.com");
        exit();
    }
    $cihaz = $_POST["cihaz"];
    // This key must match the one in arduino_code.ino
    $deviceKey = "YOUR_SECRET_API_KEY";
    if ($cihaz != $deviceKey) {
        header("Location: http://your-domain.com");
        exit();
    }
    // Use the filename specified in the README
    require_once("db_connect.php"); // Database connection file

    // TERMINATE ALL EXPIRED RESERVATIONS
    $query = $conn->query("SELECT * FROM parkyerleri WHERE durum = '2'")->fetchAll(PDO::FETCH_ASSOC); // durum '2' is reserved
    if ($query) {
        foreach ($query as $park) {
            $parkID = $park["id"];
            $baslangic = $park["rezbas"]; // reservation start time
            $suan = time(); // current time
            $gecenZaman = round(($suan - $baslangic) / 60); // elapsed time in minutes
            if ($gecenZaman >= 60) { // If reservation is older than 60 minutes
                // This query uses a variable that is safe because it comes from the database itself, not user input.
                $guncelle = $conn->exec("UPDATE parkyerleri SET durum ='1', uye='0', rezbas=NULL WHERE id = '$parkID'"); // Make it available
            }
        }
    }

    // MAKE UPDATES ACCORDING TO SENSORS (SECURE VERSION)
    $parking_statuses = [
        1 => $_POST["park1"],
        2 => $_POST["park2"],
        3 => $_POST["park3"],
        4 => $_POST["park4"]
    ];

    foreach ($parking_statuses as $park_id => $status) {
        // Ensure the incoming data is only '0' or '1' for security.
        if ($status !== '0' && $status !== '1') {
            continue; // Skip if the value is invalid.
        }

        if ($status == 1) {
            // If the space is "empty", update it only if it is not "reserved".
            $sql = "UPDATE parkyerleri SET durum = ?, uye = '0', rezbas = NULL WHERE id = ? AND durum != '2'";
            $stmt = $conn->prepare($sql);
            $stmt->execute([$status, $park_id]);
        } else {
            // If the space is "occupied", update it directly.
            $sql = "UPDATE parkyerleri SET durum = ?, uye = '0', rezbas = NULL WHERE id = ?";
            $stmt = $conn->prepare($sql);
            $stmt->execute([$status, $park_id]);
        }
    }


    // CHECK ACTIVE RESERVATIONS
    $query = $conn->query("SELECT * FROM parkyerleri WHERE durum = '2'")->fetchAll(PDO::FETCH_ASSOC);
    if ($query) {
        foreach ($query as $park) {
            $parkID = $park["id"];
            echo 'REZ' . $parkID; // Send back reservation info to Arduino
        }
    }

    // IS THERE A MEMBER LIST REQUEST? IF SO, RETURN MEMBERS
    if (isset($_POST["istek"])) {
        $query = $conn->query("SELECT * FROM uyeler")->fetchAll(PDO::FETCH_ASSOC);
        if ($query) {
            foreach ($query as $sorgu) {
                echo "'";
                echo $sorgu["kart"]; // return card numbers
                echo "'";
            }
        }
    }

    // IF THERE IS A MEMBER ENTRY, SAVE IT
    if (isset($_POST["uye"])) {
        $card_id = $_POST["uye"];
        $timestamp = time();
        $transaction_type = 1; // 1 = Entry transaction

        $sql = "INSERT INTO kayitlar (uye, tarih, tip) VALUES (?, ?, ?)";
        $stmt = $conn->prepare($sql);
        $stmt->execute([$card_id, $timestamp, $transaction_type]);
    }
} else {
    header("Location: http://your-domain.com");
    exit();
}
?>