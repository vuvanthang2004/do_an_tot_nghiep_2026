#Gọi hàm chuyên về ai để xử lý dữ liệu , nhận các con số tính toán và có được để trả về kết quả dự án
from ai.predict import predict_air_quality


def analyze_air_quality(sensor_data: dict):
  
#Nhận dữ liệu cảm biến từ sensor_service 
    try:
        temperature = float(sensor_data["temperature"])
        humidity = float(sensor_data["humidity"])
        mq2 = float(sensor_data["mq2"])
        pm25 = float(sensor_data["pm25"])

        result = predict_air_quality(
            temperature=temperature,
            humidity=humidity,
            mq2=mq2,
            pm25=pm25
        )

        return result

    except (KeyError, TypeError, ValueError) as exc:
        print("Dữ liệu cảm biến không hợp lệ:", exc)
        return None
