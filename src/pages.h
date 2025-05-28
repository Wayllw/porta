extern const char *MainPage = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.Fechado { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>Porta</h2>
          <a href="/toggle/1" class="btn green_TEXT">green_TEXT</a>
        </div>
      </body>
    </html>
  )";



extern const char *RegisterPage = R"(
  <!DOCTYPE html>
  <html lang="pt">
  <head>
    <meta charset="UTF-8">
    <title>Registo de Utilizador</title>
  </head>
  <body>

    
    <h2>Menu</h2>
    <a href="/" class="btn green_TEXT">Menu</a>

    <h2>Registar Utilizador</h2>
    <form id="registoForm">



      <label>Nome:</label><br>
      <input type="text" id="nome" required><br>
      <label>Email:</label><br>
      <input type="email" id="email" required><br>
      <label>Password:</label><br>
      <input type="password" id="password" required><br>
      <label>Confirm Password:</label><br>
      <input type="Password" id="CPassword" required><br><br>
      <button type="submit">Registar</button>
    </form>

    <p id="resposta"></p>

    <script>
      document.getElementById("registoForm").addEventListener("submit", function(e) {
        e.preventDefault();
        const nome = document.getElementById("nome").value;
        const email = document.getElementById("email").value;
        const password = document.getElementById("password").value;
        const CPassword = document.getElementById("CPassword").value;


        fetch("/registar", {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify({ nome, email, password, CPassword })
        })
        .then(response => response.text())
        .then(data => {
          document.getElementById("resposta").textContent = data;
        });
      });
    </script>
  </body>
  </html>
)";



extern const char *LoginPage = R"(
  <!DOCTYPE html>
  <html lang="pt">
  <head>
    <meta charset="UTF-8">
    <title>Login</title>
  </head>
  <body>
        
    <h2>Menu</h2>
    <a href="/" class="btn green_TEXT">Menu</a>

    <h2>Login</h2>

    <form id="loginForm">
      <label>Email</label><br>
      <input type="email" id="email" required><br>
      
      <label>Password:</label><br>
      <input type="password" id="password" required><br><br>
      
      <button type="submit">Login</button>
    </form>

    <p id="resposta"></p>

    <script>
      document.getElementById("loginForm").addEventListener("submit", function(e) {
        e.preventDefault();
        const email = document.getElementById("email").value;
        const password = document.getElementById("password").value;

        fetch("/login", {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify({ email, password })
        })
        .then(response => response.text().then(data => {
          if (response.status === 200) {
            window.location.href = "/main";
          } else {
            document.getElementById("resposta").textContent = data;
          }
        }));
      });
      </script>
  </body>
  </html>
)";




extern const char *Menu = R"(
    <!DOCTYPE html><html>
      <head>
        <title>Fechadura</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #333; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
        </style>
      </head>
            
      <body>
        <h1>Fechadura Web Server</h1>

        <div>
          <h2>Registo</h2>
          <a href="/registar" class="btn green_TEXT">Registo</a>

          <h2>Login</h2>
          <a href="/login" class="btn green_TEXT">Login</a>
        </div>
      </body>
    </html>
  )";