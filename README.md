# Physical AI Vibe Coding Platform

바이브 코딩으로 AI 웹앱을 만들고, micro:bit와 ESP32를 통해 실제 장치를 제어하는 교육용 피지컬 AI 플랫폼입니다.

## 프로젝트 구성

```text
.
├── physical/                 # 하드웨어 동작 및 통신
│   ├── microbit/             # MakeCode 확장
│   ├── esp32/                # ESP32 펌웨어
│   └── bridge-server/        # 웹앱과 ESP32 사이의 통신 서버
├── webapp/                   # 바이브 코딩으로 만들 AI 웹앱
└── tutorials/                # 단계별 실습 자료
```

## 구성 요소의 역할

- `physical/microbit`: 블록 코딩 프로그램과 ESP32 사이의 UART 통신 및 모터 제어 명령을 담당합니다.
- `physical/esp32`: Wi-Fi, 서버 통신, OLED 표시, TB6612FNG 모터 구동을 담당합니다.
- `physical/bridge-server`: 웹 요청과 ESP32 TCP 연결을 중계하는 Flask 서버입니다.
- `webapp`: 바이브 코딩으로 새 AI 웹페이지를 만드는 공간입니다. 현재는 비어 있습니다.
- `tutorials`: 설치, 연결, AI 웹앱 제작 및 피지컬 장치 연동 과정을 문서화하는 공간입니다.

각 구성 요소의 실행 및 배선 방법은 해당 폴더의 README를 참고하세요.
