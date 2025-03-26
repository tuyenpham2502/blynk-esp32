const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
<meta charset="utf-8">

<head>
  <title>🌿HỆ THỐNG GIÁM SÁT Ô NHIỄM KHÔNG KHÍ🌿</title>
  <style>
    body {
      background-color: rgb(241, 241, 241);
      padding: 10px;
    }
    .container {     
      display: flex;
      justify-content: center; 
      align-items: center; 
      flex-direction: column;
      padding-top: 10px;
      font-family: Monospace;
       
    }
    .button {
      background-color: #04AA6D; /* Green */
      border: none;
      color: white;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: x-large;
      margin: 10px 2px;
      cursor: pointer;
      width: 30%;
      border-radius: 5px;
    }
    input[type=text], select {
      width: 100%;
      padding: 12px 20px;
      margin: 8px 0;
      display: inline-block;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
      font-size: xx-large;
      height : 85px;
    }
    input[type=number].tempThre , input[type=number].humiThre , 
    input[type=number].dustThre {
      width: 30%;
      padding: 12px 20px;
      margin: 8px 2px;
      display: inline-block;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
      font-size: xx-large;
      height : 85px;
    }

    .submit {
      width: 100%;
      background-color:#04AA6D;
      color: white;
      padding: 14px 20px;
      margin: 8px 0;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }

    .submit:hover, .button:hover {
      background-color: #989b98;
    }
    .container-2 {
      display: flex;
      margin-bottom: 20px;
      justify-content: start;
      gap: 10px;
    }

    h1 {
      text-align: center;
      margin-bottom: 40px;
      font-size: 40px;
    }
    h2 {
      font-size: 20px;
    }
    h4 {
      font-size: 15px;
    }
    p {
      font-size: 10px;
    }
    @media (min-width: 300px) and (max-width: 900px) {
      .container {     
        margin-right: 0%;
        margin-left: 0%;
      }
    }
   
  </style>
</head>

<body>
  <div class="container">
      <h1>🌿HỆ THỐNG GIÁM SÁT Ô NHIỄM KHÔNG KHÍ🌿</h1>
      <div>
        <hr>
        <h2>Cấu hình WIFI</h2>
        <div>
          <h4>Tên WIFI </h4>
          <input type="text" id="ssid" name="ssid" placeholder="Your ssid..">
      
          <h4>Mật khẩu</h4>
          <input type="text" id="pass" name="pass" placeholder="Your password ..">

          <h4>Mã Token Blynk</h4>
          <input type="text" id="token" name="token" placeholder="Your Token Blynk ..">
          
          <hr>

          <h2>Cấu hình ngưỡng</h2>
          <h4>Ngưỡng nhiệt độ môi trường (*C)</h4>
          <p>🌞Ngưỡng 1 &lt; Khoảng an toàn &lt; Ngưỡng 2</p>
     

          <div class="container-2">
            <input class="tempThre" type="number" id="tempThreshold1" name="tempThreshold1" min="10" max="100" step="1" placeholder="Ngưỡng 1">
            <input class="tempThre" type="number" id="tempThreshold2" name="tempThreshold2" min="10" max="100" step="1" placeholder="Ngưỡng 2">  
          </div>
          
 
          <h4>Ngưỡng độ ẩm không khí (%)</h4>
          <p>🌱Ngưỡng 1 &lt; Khoảng an toàn &lt; Ngưỡng 2</p>
 
          <div class="container-2">
            <input class="humiThre" type="number" id="humiThreshold1" name="humiThreshold1" min="10" max="100" step="1"  placeholder="Ngưỡng 1">
            <input class="humiThre" type="number" id="humiThreshold2" name="humiThreshold2" min="10" max="100" step="1"  placeholder="Ngưỡng 2">  
          </div>
          


          <h4>Ngưỡng cảm biến bụi (%)</h4>
          <p>🍁Khoảng an toàn &lt; Ngưỡng 1 &lt; Ngưỡng 2</p>
   
          <div class="container-2">
            <input class="dustThre" type="number" id="dustThreshold1" name="dustThreshold1" placeholder="Ngưỡng 1">
            <input class="dustThre" type="number" id="dustThreshold2" name="dustThreshold2" placeholder="Ngưỡng 2">  
            
          </div>
          <div class="container-2">
            <button class="submit" id="btnDefauld" ><h2 style="font-size: 30px;">Chọn mặc định</h2></button>
            <button class="submit" id="btnSubmit"><h2 style="font-size: 30px;">Gửi</h2></button>
          </div>
          
        </div>
      </div>
      <div>
         
      </div>
    
  </div>

  <script>
    var data = {
          ssid   : "",
          pass   : "",
          token : "",
          tempThreshold1 : "",
          tempThreshold2 : "",
          humiThreshold1 : "",
          humiThreshold2 : "",
          dustThreshold1 : "",
          dustThreshold2 : "",
    };
    const ssid   = document.getElementById("ssid");
    const pass   = document.getElementById("pass");
    const token = document.getElementById("token");

    const tempThreshold1 =  document.getElementsByName('tempThreshold1')[0];
    const tempThreshold2 =  document.getElementsByName('tempThreshold2')[0];

    const humiThreshold1 = document.getElementsByName('humiThreshold1')[0];
    const humiThreshold2 = document.getElementsByName('humiThreshold2')[0];

    const dustThreshold1 = document.getElementsByName('dustThreshold1')[0];
    const dustThreshold2 = document.getElementsByName('dustThreshold2')[0];
    const btnDefauld= document.getElementById("btnDefauld");

    var xhttp = new XMLHttpRequest();
    xhttp.open("GET","/data_before", true),
    xhttp.send();
    xhttp.onreadystatechange = function() {
      if(xhttp.readyState == 4 && xhttp.status == 200) {
        //alert(this.responseText);
        const obj    = JSON.parse(this.responseText); // chuyển JSON sang JS Object
        //alert(obj.ssid);
        ssid.value   = obj.ssid;
        pass.value   = obj.pass;
        token.value = obj.token;
        tempThreshold1.value = obj.tempThreshold1;
        tempThreshold2.value = obj.tempThreshold2;
        humiThreshold1.value = obj.humiThreshold1;
        humiThreshold2.value = obj.humiThreshold2;
        dustThreshold1.value = obj.dustThreshold1;
        dustThreshold2.value = obj.dustThreshold2;
      }
    }
    
    btnDefauld.addEventListener("click", function(event) {
        tempThreshold1.value = 20;
        tempThreshold2.value = 32;
        humiThreshold1.value = 40;
        humiThreshold2.value = 75;
        dustThreshold1.value = 40;
        dustThreshold2.value = 150;
    });

    var xhttp2 = new XMLHttpRequest();
    const btnSubmit = document.getElementById("btnSubmit"); 
    btnSubmit.addEventListener('click', () => { 
        data = {
          ssid   : ssid.value,
          pass   : pass.value,
          token :  token.value,
          tempThreshold1 : Number(tempThreshold1.value),
          tempThreshold2 : Number(tempThreshold2.value),
          humiThreshold1 : Number(humiThreshold1.value),
          humiThreshold2 : Number(humiThreshold2.value),
          dustThreshold1 : Number(dustThreshold1.value),
          dustThreshold2 : Number(dustThreshold2.value)
        }
        
        xhttp2.open("POST","/post_data", true),
        xhttp2.send(JSON.stringify(data)); // chuyển JSObject sang JSON để gửi về server
        xhttp2.onreadystatechange = function() {
          if(xhttp2.readyState == 4 && xhttp2.status == 200) {
            alert("Cài đặt thành công");
          } 
        }
    });
     
  </script>
</body>
</html>

)rawliteral";

