
from contextlib import asynccontextmanager

from fastapi import FastAPI
from backend import mqtt_client


@asynccontextmanager
async def lifespan(app: FastAPI):
    mqtt_client.start_mqtt()
    try:
        yield
    finally:
        mqtt_client.stop_mqtt()


app = FastAPI(
    title="Air Quality Monitoring Backend",
    lifespan=lifespan,
)


@app.get("/")
def root():
    return {"message": "Air Quality Backend is running"}


@app.get("/api/sensors")
def get_sensors():
    return mqtt_client.sensor_data


@app.get("/api/devices")
def get_devices():
    return mqtt_client.device_status
