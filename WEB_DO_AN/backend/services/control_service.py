
import logging

logger = logging.getLogger(__name__)


class ControlService:
    """
    Quản lý chế độ AUTO/MANUAL và quyết định điều khiển thiết bị.
    """

    def __init__(self):

        # Vừa khởi động thì nó sẽ ở trạng thái auto
        self.current_mode = "AUTO"
        # Trạng thái thiết bị ban đầu sẽ là 0, chưa bật
        self.device_state = {
            "fan": 0,
            "tec": 0,
            "ion": 0
        }

#Hàm này cho phép người dùng chuyển giữa hai chế độ
    def set_mode(self, mode: str):

        mode = mode.upper()

        if mode not in ["AUTO", "MANUAL"]:
            logger.warning("Chế độ không hợp lệ: %s", mode)
            return False

        self.current_mode = mode

        logger.info("Chuyển chế độ: %s", mode)

        return True

    # ==========================================
    # Điều khiển thủ công
    # ==========================================

    def manual_control(self, command: dict):

        if self.current_mode != "MANUAL":
            logger.warning("Không thể điều khiển thủ công khi ở AUTO")
            return None

        for device in ["fan", "tec", "ion"]:

            if device in command:

                value = command[device]

                if value not in [0, 1, False, True]:
                    logger.warning("Lệnh không hợp lệ: %s", device)
                    return None

                self.device_state[device] = int(value)

        logger.info("Trạng thái MANUAL: %s", self.device_state)

        return {
            "mode": self.current_mode,
            **self.device_state
        }

    # ==========================================
    # Điều khiển tự động
    # ==========================================
    def auto_control(self, ai_result: dict, sensor_data: dict):

        if self.current_mode != "AUTO":
            return None

        air_quality = ai_result.get("air_quality")

        temperature = float(sensor_data["temperature"])

        # Mặc định tắt thiết bị
        fan = 0
        tec = 0
        ion = 0

        # Không khí kém
        if air_quality == "poor":

            fan = 1
            ion = 1

        # Không khí trung bình
        elif air_quality == "moderate":

            fan = 1

        # Không khí tốt
        elif air_quality == "good":

            fan = 0
            ion = 0

        # Điều khiển TEC theo nhiệt độ
        if temperature > 25:

            tec = 1

        self.device_state = {
            "fan": fan,
            "tec": tec,
            "ion": ion
        }

        result = {
            "mode": self.current_mode,
            **self.device_state
        }

        logger.info("AUTO control: %s", result)

        return result
