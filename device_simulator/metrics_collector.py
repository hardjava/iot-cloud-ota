import json
import logging
import random
import threading
import time
from datetime import datetime, timezone
from typing import Dict, List

import constants
from mqtt_client import MqttClient

logger = logging.getLogger(__name__)


class MetricsCollector:
    """주기적으로 시스템 메트릭을 수집하고 MQTT로 보고하는 클래스."""

    def __init__(self, device_id: str, mqtt_client: MqttClient):
        """MetricsCollector를 초기화합니다.

        Args:
            device_id: 시뮬레이션할 디바이스의 고유 ID.
            mqtt_client: MQTT 통신을 위한 클라이언트 인스턴스.
        """
        self._device_id = device_id
        self._mqtt_client = mqtt_client
        self._start_time = time.time()
        self._advertisements: List[Dict[str, int]] = []
        self._thread = None
        self._stop_event = threading.Event()

    def start(self):
        """메트릭 수집 및 보고를 시작합니다. (백그라운드 스레드)"""
        if self._thread is None:
            self._thread = threading.Thread(target=self._run_collector)
            self._thread.daemon = True
            self._thread.start()
            logger.info("Metrics collector has been started.")

    def stop(self):
        """메트릭 수집을 중지합니다."""
        self._stop_event.set()
        if self._thread:
            self._thread.join()
        logger.info("Metrics collector has been stopped.")

    def update_advertisements(self, ads: List[Dict[str, int]]):
        """현재 디바이스에 설치된 광고 목록을 업데이트합니다.

        Args:
            ads: 광고 ID 목록 (e.g., [{'id': 1}, {'id': 2}])
        """
        logger.info("Updating advertisements list for metrics: %s", ads)
        self._advertisements = ads

    def _run_collector(self):
        """10초 간격으로 메트릭을 수집하고 보고하는 메인 루프."""
        while not self._stop_event.is_set():
            try:
                metrics = self._collect_metrics()
                payload = json.dumps(metrics)
                topic = constants.SYSTEM_STATUS_TOPIC.format(device_id=self._device_id)
                self._mqtt_client.publish(topic, payload)
                logger.debug("System metrics published successfully.")
            except Exception as e:
                logger.error("Failed to collect or publish metrics: %s", e)

            # 10초 대기 (stop 이벤트를 확인하며)
            self._stop_event.wait(10)

    def _collect_metrics(self) -> dict:
        """시뮬레이션된 시스템 및 네트워크 메트릭을 수집합니다."""
        uptime = int(time.time() - self._start_time)

        # 실제 하드웨어에서 가져오는 것처럼 값을 시뮬레이션
        cpu_usage = {
            "core_0": round(random.uniform(5.0, 30.0), 1),
            "core_1": round(random.uniform(5.0, 30.0), 1),
        }
        memory_usage = round(random.uniform(60.0, 75.0), 1)
        storage_usage = round(random.uniform(35.0, 37.0), 1)  # 비교적 정적인 값 유지

        network = {
            "connection_type": "wifi",
            "signal_strength": random.randint(-55, -40),
            "local_ip": "192.168.1.100",
            "gateway_ip": "192.168.1.1",
        }

        return {
            "firmware_version": constants.FIRMWARE_VERSION,
            "advertisements": self._advertisements,
            "system": {
                "cpu_usage": cpu_usage,
                "memory_usage": memory_usage,
                "storage_usage": storage_usage,
                "uptime": uptime,
            },
            "network": network,
            "timestamp": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        }
