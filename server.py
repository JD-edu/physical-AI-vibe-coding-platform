from flask import (
    Flask,
    request,
    render_template_string,
    redirect,
    url_for
)
from datetime import datetime
from threading import Lock, Thread
import socket

app = Flask(__name__)

# ==================================================
# 설정
# ==================================================

FLASK_PORT = 5000
TCP_PORT = 5001

# ==================================================
# 수신 데이터
# ==================================================

latest_message = "아직 수신된 데이터가 없습니다."
latest_time = "-"

message_history = []

# 마지막으로 서버에서 보낸 명령
latest_command = "없음"
latest_command_time = "-"

data_lock = Lock()

# 연결된 ESP32 TCP 소켓 목록
esp32_clients = []
clients_lock = Lock()

# ==================================================
# 웹페이지
# ==================================================

HTML_PAGE = """
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">

    <meta name="viewport"
          content="width=device-width, initial-scale=1.0">

    <title>ESP32 양방향 통신 서버</title>

    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f2f4f7;
            margin: 0;
            padding: 20px;
        }

        .container {
            max-width: 750px;
            margin: 30px auto;
        }

        h1 {
            color: #1565c0;
            text-align: center;
        }

        .card {
            background-color: white;
            margin-bottom: 20px;
            padding: 25px;
            border-radius: 15px;
            box-shadow:
                0 4px 12px rgba(0, 0, 0, 0.12);
        }

        .value-box {
            padding: 20px;
            border-radius: 10px;
            background-color: #e8f5e9;
            font-size: 24px;
            font-weight: bold;
            word-break: break-all;
        }

        .command-box {
            padding: 20px;
            border-radius: 10px;
            background-color: #e3f2fd;
            font-size: 36px;
            font-weight: bold;
            text-align: center;
        }

        form {
            display: flex;
            gap: 10px;
            margin-top: 20px;
        }

        input {
            flex: 1;
            padding: 14px;
            border: 1px solid #cccccc;
            border-radius: 8px;
            font-size: 22px;
        }

        button {
            padding: 14px 22px;
            border: none;
            border-radius: 8px;
            background-color: #1565c0;
            color: white;
            font-size: 17px;
            cursor: pointer;
        }

        .time {
            margin-top: 10px;
            color: #555555;
        }

        .status {
            margin-top: 10px;
            color: #1565c0;
            font-weight: bold;
        }

        table {
            width: 100%;
            border-collapse: collapse;
        }

        th,
        td {
            padding: 12px;
            border-bottom: 1px solid #dddddd;
            text-align: left;
        }

        th {
            background-color: #eeeeee;
        }
    </style>
</head>

<body>
    <div class="container">

        <h1>ESP32 양방향 통신 서버</h1>

        <div class="card">
            <h2>마이크로비트로 문자 보내기</h2>

            <div class="command-box">
                {{ latest_command }}
            </div>

            <div class="time">
                전송 시간: {{ latest_command_time }}
            </div>

            <div class="status">
                연결된 ESP32:
                {{ connected_clients }}대
            </div>

            <form
                method="post"
                action="/send-command"
            >
                <input
                    type="text"
                    name="command"
                    maxlength="20"
                    placeholder="보낼 문자 또는 문자열"
                    required
                    autofocus
                >

                <button type="submit">
                    즉시 전송
                </button>
            </form>
        </div>

        <div class="card">
            <h2>마이크로비트에서 받은 데이터</h2>

            <div class="value-box">
                {{ latest_message }}
            </div>

            <div class="time">
                수신 시간: {{ latest_time }}
            </div>
        </div>

        <div class="card">
            <h2>수신 기록</h2>

            <table>
                <thead>
                    <tr>
                        <th>시간</th>
                        <th>데이터</th>
                    </tr>
                </thead>

                <tbody>
                    {% for item in message_history %}
                    <tr>
                        <td>{{ item.time }}</td>
                        <td>{{ item.message }}</td>
                    </tr>
                    {% endfor %}
                </tbody>
            </table>
        </div>

    </div>
</body>
</html>
"""

# ==================================================
# 연결된 ESP32 개수
# ==================================================

def get_client_count():
    with clients_lock:
        return len(esp32_clients)

# ==================================================
# ESP32에 명령 전송
# ==================================================

def broadcast_command(command):
    """
    연결된 모든 ESP32에 명령을 즉시 보냅니다.
    ESP32는 줄바꿈을 기준으로 명령을 구분합니다.
    """

    packet = (command + "\n").encode("utf-8")

    disconnected_clients = []

    with clients_lock:
        for client_socket in esp32_clients:
            try:
                client_socket.sendall(packet)

            except (ConnectionError, OSError):
                disconnected_clients.append(
                    client_socket
                )

        for client_socket in disconnected_clients:
            if client_socket in esp32_clients:
                esp32_clients.remove(
                    client_socket
                )

            try:
                client_socket.close()
            except OSError:
                pass

    return (
        len(esp32_clients),
        len(disconnected_clients)
    )

# ==================================================
# ESP32 TCP 연결 처리
# ==================================================

def handle_esp32_client(
    client_socket,
    client_address
):
    print(
        "ESP32 TCP 연결:",
        client_address
    )

    with clients_lock:
        esp32_clients.append(client_socket)

    try:
        while True:
            # ESP32의 연결 상태와 초기 메시지 확인
            data = client_socket.recv(1024)

            if not data:
                break

            text = data.decode(
                "utf-8",
                errors="replace"
            ).strip()

            if text:
                print(
                    "ESP32 TCP 메시지:",
                    client_address,
                    text
                )

    except (ConnectionError, OSError):
        pass

    finally:
        with clients_lock:
            if client_socket in esp32_clients:
                esp32_clients.remove(
                    client_socket
                )

        try:
            client_socket.close()
        except OSError:
            pass

        print(
            "ESP32 TCP 연결 종료:",
            client_address
        )

# ==================================================
# TCP 서버 실행
# ==================================================

def run_tcp_server():
    server_socket = socket.socket(
        socket.AF_INET,
        socket.SOCK_STREAM
    )

    server_socket.setsockopt(
        socket.SOL_SOCKET,
        socket.SO_REUSEADDR,
        1
    )

    server_socket.bind(
        ("0.0.0.0", TCP_PORT)
    )

    server_socket.listen(10)

    print(
        f"ESP32 TCP server started: "
        f"0.0.0.0:{TCP_PORT}"
    )

    while True:
        client_socket, client_address = (
            server_socket.accept()
        )

        client_thread = Thread(
            target=handle_esp32_client,
            args=(
                client_socket,
                client_address
            ),
            daemon=True
        )

        client_thread.start()

# ==================================================
# 메인 웹페이지
# ==================================================

@app.route("/", methods=["GET"])
def home():
    with data_lock:
        current_message = latest_message
        current_time = latest_time

        current_command = latest_command
        current_command_time = (
            latest_command_time
        )

        current_history = list(
            reversed(message_history)
        )

    return render_template_string(
        HTML_PAGE,
        latest_message=current_message,
        latest_time=current_time,
        latest_command=current_command,
        latest_command_time=(
            current_command_time
        ),
        message_history=current_history,
        connected_clients=get_client_count()
    )

# ==================================================
# ESP32에서 올라오는 데이터 수신
# ==================================================

@app.route("/receive", methods=["POST"])
def receive_data():
    global latest_message
    global latest_time

    received_data = request.get_data(
        as_text=True
    ).strip()

    if received_data == "":
        return "EMPTY_DATA", 400

    receive_time = datetime.now().strftime(
        "%Y-%m-%d %H:%M:%S"
    )

    with data_lock:
        latest_message = received_data
        latest_time = receive_time

        message_history.append({
            "time": receive_time,
            "message": received_data
        })

        if len(message_history) > 20:
            message_history.pop(0)

    print("--------------------------------")
    print("마이크로비트 데이터 수신")
    print("시간:", receive_time)
    print("데이터:", received_data)
    print("ESP32 IP:", request.remote_addr)
    print("--------------------------------")

    return "OK", 200

# ==================================================
# 웹에서 ESP32로 명령 즉시 전송
# ==================================================

@app.route("/send-command", methods=["POST"])
def send_command():
    global latest_command
    global latest_command_time

    command = request.form.get(
        "command",
        ""
    ).strip()

    if command == "":
        return redirect(url_for("home"))

    sent_count, removed_count = (
        broadcast_command(command)
    )

    send_time = datetime.now().strftime(
        "%Y-%m-%d %H:%M:%S"
    )

    with data_lock:
        latest_command = command
        latest_command_time = send_time

    print("--------------------------------")
    print("서버 명령 전송")
    print("시간:", send_time)
    print("명령:", command)
    print("연결된 ESP32:", sent_count)
    print("끊어진 연결 제거:", removed_count)
    print("--------------------------------")

    return redirect(url_for("home"))

# ==================================================
# 실행
# ==================================================

if __name__ == "__main__":
    tcp_thread = Thread(
        target=run_tcp_server,
        daemon=True
    )

    tcp_thread.start()

    app.run(
        host="0.0.0.0",
        port=FLASK_PORT,
        debug=False,
        threaded=True
    )