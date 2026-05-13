from flask import Flask, request, jsonify
import requests

# Flask 앱 생성
app = Flask(__name__)

# 🔑 Gemini API 설정
API_KEY = "AIzaSyB84P1bMGHxTLYHC8NK5zGE4IiHEv5bsJY"  # 👉 여기에 Gemini API 키 입력
GEMINI_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent"



# 📡 1. ESP32에서 오는 센서 데이터 처리 API

@app.route("/home")
def home():
    return "집에갈래"
@app.route("/sensor")
def sensor():
    try:
        
        # 📥 1. 요청(JSON)에서 센서 데이터 추출
        
        # data = request.get_json(force=True)

        temp = 25   # 온도
        hum = 50      # 습도

        print(f"[센서] 온도: {temp}, 습도: {hum}")

        
        # 🤖 2. Gemini에 보낼 프롬프트 생성
        
        prompt = f"""
현재 온도 {temp}도, 습도 {hum}%입니다.
이 환경에 맞는 짧은 어린이 옷차림 조언을 20자 이내 한국어로 해줘.
"""

        
        # 🌐 3. Gemini API 요청 준비
        
        headers = {
            "Content-Type": "application/json",
            "x-goog-api-key": API_KEY,  # 인증 키
        }

        body = {
            "contents": [
                {
                    "parts": [
                        {"text": prompt}  # 모델에게 전달할 텍스트
                    ],
                }
            ]
        }

        
        # 🚀 4. Gemini API 호출
        
        res = requests.post(
            GEMINI_URL,
            headers=headers,
            json=body,
            timeout=10   # 최대 대기 시간 (초)
        )

        # JSON 응답 변환
        result_json = res.json()

        
        # 🔍 5. 응답에서 텍스트 추출
        
        result = result_json["candidates"][0]["content"]["parts"][0]["text"]

        print(f"[AI 응답] {result}")

        
        # 📤 6. ESP32로 결과 반환
        
        return jsonify({"result": result})

    
    # ❌ 에러 처리
    
    except Exception as e:
        print("에러:", e)
        return jsonify({"error": str(e)})



# 🖥️ 7. 서버 실행

if __name__ == "__main__":
    # 외부(ESP32)에서 접속 가능하도록 0.0.0.0 사용
    app.run(host="0.0.0.0", port=5000)