
import logging

logger = logging.getLogger(__name__)


class DeviceService:
    """
    Quản lý trạng thái thiết bị và lệnh điều khiển.
    """

    def __init__(self):
        self.latest_status = {
            "fan": 0,
            "tec": 0,
            "ion": 0
        }

        self.last_command = None

    def process_device_status(self, data: dict):
        """
        Xử lý trạng thái thiết bị nhận từ ESP32 qua MQTT.
        """
        if not isinstance(data, dict):
            logger.error("Dữ liệu trạng thái thiết bị không hợp lệ")
            return None

        try:
            status = {}

            for device in ["fan", "tec", "ion"]:
                value = data.get(device, self.latest_status[device])

                if value not in [0, 1, False, True]:
                    logger.warning("Trạng thái %s không hợp lệ", device)
                    return None

                status[device] = int(value)

            self.latest_status.update(status)

            logger.info("Trạng thái thiết bị: %s", self.latest_status)

            return self.latest_status.copy()

        except (TypeError, ValueError) as exc:
            logger.error("Lỗi xử lý trạng thái thiết bị: %s", exc)
            return None

    def update_command(self, command: dict):
        """
        Lưu lại lệnh điều khiển vừa gửi đến ESP32.
        """
        if not isinstance(command, dict):
            logger.error("Lệnh điều khiển không hợp lệ")
            return None

        self.last_command = command.copy()

        logger.info("Lệnh điều khiển: %s", self.last_command)

        return self.last_command.copy()

    def get_latest_status(self):
        return self.latest_status.copy()

    def get_last_command(self):
        return self.last_command
