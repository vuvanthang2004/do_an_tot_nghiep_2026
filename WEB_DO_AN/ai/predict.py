
import pickle
from pathlib import Path

import pandas as pd


# ==========================================
# 1. Đường dẫn mô hình AI
# ==========================================

BASE_DIR = Path(__file__).resolve().parent

MODEL_PATH = BASE_DIR / "air_quality_model.pkl"


# ==========================================
# 2. Load mô hình đã huấn luyện
# ==========================================

with open(MODEL_PATH, "rb") as file:
    model = pickle.load(file)

print("Load mô hình AI thành công!")


# ==========================================
# 3. Hàm dự đoán chất lượng không khí
# ==========================================

def predict_air_quality(
    temperature,
    humidity,
    mq2,
    pm25
):

    # Dữ liệu đầu vào
    data = pd.DataFrame(
        [[temperature, humidity, mq2, pm25]],
        columns=[
            "temperature",
            "humidity",
            "mq2",
            "pm25"
        ]
    )

    # Dự đoán
    prediction = model.predict(data)[0]

    # Xác suất dự đoán của từng lớp
    probabilities = model.predict_proba(data)[0]

    # Lấy tên các lớp
    classes = model.classes_

    confidence = max(probabilities) * 100

    return {
        "air_quality": prediction,
        "confidence": round(confidence, 2)
    }


# ==========================================
# 4. Thử nghiệm mô hình
# ==========================================

if __name__ == "__main__":

    result = predict_air_quality(
        temperature=28.5,
        humidity=65,
        mq2=45,
        pm25=25
    )

    print("\nKết quả dự đoán:")
    print("Chất lượng không khí:", result["air_quality"])
    print("Độ tin cậy mô hình:", result["confidence"], "%")
