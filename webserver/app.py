from flask import Flask, render_template

app = Flask(__name__)

@app.route('/')
def index():
    # templates/index.html 파일을 불러와 브라우저에 렌더링합니다.
    return render_template('index.html')

if __name__ == '__main__':
    print("=" * 60)
    print("🤖 피지컬 AI Teachable Machine 서버가 시작되었습니다.")
    print("👉 크롬 또는 엣지 브라우저에서 아래 주소로 접속하세요:")
    print("   http://127.0.0.1:5000  또는  http://localhost:5000")
    print("=" * 60)
    
    # debug=True 모드로 실행하여 코드 변경 시 자동 재시작
    app.run(host='0.0.0.0', port=5000, debug=True)