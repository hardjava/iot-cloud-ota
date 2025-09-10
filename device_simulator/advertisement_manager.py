import dataclasses
import hashlib
import json
import logging
import threading
import time
from datetime import datetime, timezone
from typing import List

import constants
import http_client as http
import mqtt_client as mqtt
from constants import RequestStatus, ResultStatus
from metrics_collector import MetricsCollector

logger = logging.getLogger(__name__)


@dataclasses.dataclass
class AdContent:
    """광고 콘텐츠 정보를 담는 데이터 클래스"""

    signed_url: str
    checksum: str
    size: int
    timeout: int
    file_id: int


@dataclasses.dataclass
class AdvertisementDownloadRequest:
    """광고 다운로드 요청 메시지 스키마 (Subscribe)"""

    command_id: str
    contents: List[AdContent]
    timestamp: str


@dataclasses.dataclass
class AdvertisementDownloadAck:
    """광고 다운로드 요청 ACK 메시지 스키마 (Publish)"""

    command_id: str
    status: str
    timestamp: str = dataclasses.field(
        default_factory=lambda: datetime.now(timezone.utc)
        .isoformat()
        .replace("+00:00", "Z")
    )


@dataclasses.dataclass
class AdvertisementDownloadProgress:
    """광고 다운로드 진행 상태 메시지 스키마 (Publish)"""

    command_id: str
    progress: int
    downloaded_bytes: int
    total_bytes: int
    speed_kbps: float
    timestamp: str = dataclasses.field(
        default_factory=lambda: datetime.now(timezone.utc)
        .isoformat()
        .replace("+00:00", "Z")
    )


@dataclasses.dataclass
class AdvertisementDownloadResult:
    """광고 다운로드 결과 메시지 스키마 (Publish)"""

    command_id: str
    status: str
    message: str
    checksum_verified: bool
    download_ms: int
    timestamp: str = dataclasses.field(
        default_factory=lambda: datetime.now(timezone.utc)
        .isoformat()
        .replace("+00:00", "Z")
    )


class AdvertisementManager:
    """광고 다운로드 및 업데이트 프로세스의 전체 로직을 관리하는 클래스."""

    def __init__(
        self,
        device_id: str,
        mqtt_client: mqtt.MqttClient,
        http_client: http.HttpClient,
        metrics_collector: MetricsCollector,
    ):
        """AdvertisementManager를 초기화합니다.

        Args:
            device_id: 시뮬레이션할 디바이스의 고유 ID.
            mqtt_client: MQTT 통신을 위한 클라이언트 인스턴스.
            http_client: 파일 다운로드를 위한 클라이언트 인스턴스.
            metrics_collector: 메트릭 수집기 인스턴스.
        """
        self._device_id = device_id
        self._mqtt_client = mqtt_client
        self._http_client = http_client
        self._metrics_collector = metrics_collector
        self._download_thread = None
        logger.info("AdvertisementManager has been initialized successfully.")

    def handle_download_request(self, topic: str, payload: bytes) -> None:
        """광고 다운로드 요청 MQTT 메시지를 처리하는 콜백 함수.

        메시지를 파싱하고, ACK를 전송한 후, 별도의 스레드에서 다운로드를 시작합니다.
        """
        logger.info("Received advertisement download request on topic '%s'", topic)
        try:
            data = json.loads(payload)

            contents = []
            for content_data in data["contents"]:
                signed_url_info = content_data["signed_url"]
                file_info = content_data["file_info"]
                contents.append(
                    AdContent(
                        signed_url=signed_url_info["url"],
                        checksum=file_info["file_hash"],
                        size=file_info["size"],
                        timeout=signed_url_info["timeout"],
                        file_id=file_info["id"],
                    )
                )

            download_request = AdvertisementDownloadRequest(
                command_id=data["command_id"],
                contents=contents,
                timestamp=data["timestamp"],
            )
        except (json.JSONDecodeError, TypeError, KeyError) as e:
            logger.error("Invalid advertisement download request payload: %s", e)
            return

        if self._download_thread and self._download_thread.is_alive():
            logger.info(
                "Advertisement download is already in progress. Ignoring new request %s.",
                download_request.command_id,
            )
            return

        self._send_ack(download_request.command_id)

        self._download_thread = threading.Thread(
            target=self._download_advertisements, args=(download_request,)
        )
        self._download_thread.start()

    def _send_ack(self, command_id: str) -> None:
        """광고 다운로드 요청에 대한 ACK 메시지를 MQTT로 발행합니다."""
        ack_message = AdvertisementDownloadAck(
            command_id=command_id,
            status=RequestStatus.ACKNOWLEDGED.value,
        )
        self._mqtt_client.publish(
            constants.DOWNLOAD_ACK_TOPIC.format(device_id=self._device_id),
            json.dumps(dataclasses.asdict(ack_message)),
        )

    def _send_progress(
        self,
        command_id: str,
        downloaded_bytes: int,
        total_bytes: int,
        start_time: float,
    ) -> None:
        """광고 다운로드 진행 상황을 MQTT로 발행합니다."""
        if total_bytes == 0:
            return

        progress = int((downloaded_bytes / total_bytes) * 100)
        elapsed_time = time.time() - start_time
        speed_bps = (downloaded_bytes / elapsed_time) if elapsed_time > 0 else 0
        speed_kbps = speed_bps / 1024

        progress_message = AdvertisementDownloadProgress(
            command_id=command_id,
            progress=progress,
            downloaded_bytes=downloaded_bytes,
            total_bytes=total_bytes,
            speed_kbps=round(speed_kbps, 2),
        )
        self._mqtt_client.publish(
            constants.DOWNLOAD_PROGRESS_TOPIC.format(
                device_id=self._device_id
            ),
            json.dumps(dataclasses.asdict(progress_message)),
        )

    def _send_result(
        self,
        command_id: str,
        status: ResultStatus,
        message: str,
        checksum_verified: bool,
        download_ms: int,
    ) -> None:
        """광고 다운로드 최종 결과를 MQTT로 발행합니다."""
        result_message = AdvertisementDownloadResult(
            command_id=command_id,
            status=status.value,
            message=message,
            checksum_verified=checksum_verified,
            download_ms=download_ms,
        )
        self._mqtt_client.publish(
            constants.DOWNLOAD_RESULT_TOPIC.format(device_id=self._device_id),
            json.dumps(dataclasses.asdict(result_message)),
        )

    def _verify_checksum(self, file_path: str, expected_checksum: str) -> bool:
        """다운로드된 파일의 체크섬을 검증합니다."""
        if ":" in expected_checksum:
            algo, expected_checksum = expected_checksum.split(":", 1)
            if algo.lower() != "sha256":
                logger.error("Unsupported checksum algorithm: %s", algo)
                return False

        hasher = hashlib.sha256()
        try:
            with open(file_path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hasher.update(chunk)
            actual_hash = hasher.hexdigest()
            return actual_hash == expected_checksum
        except FileNotFoundError:
            logger.error("File not found for checksum verification: %s", file_path)
            return False
        except Exception as e:
            logger.error("Error during checksum verification: %s", e)
            return False

    def _download_advertisements(
        self, download_request: AdvertisementDownloadRequest
    ) -> None:
        """HTTP 클라이언트를 사용하여 광고 파일들을 다운로드하는 전체 프로세스."""
        start_time = time.time()

        total_size = sum(content.size for content in download_request.contents)
        total_downloaded_so_far = 0

        status: ResultStatus = ResultStatus.FAILED
        message = ""
        all_checksums_verified = True

        try:
            for ad_content in download_request.contents:

                def progress_callback(downloaded_for_this_file):
                    self._send_progress(
                        download_request.command_id,
                        total_downloaded_so_far + downloaded_for_this_file,
                        total_size,
                        start_time,
                    )

                file_path = self._http_client.download_file_with_progress(
                    signed_url=ad_content.signed_url,
                    total_size=ad_content.size,
                    progress_callback=progress_callback,
                    timeout=ad_content.timeout,
                )

                is_valid = self._verify_checksum(file_path, ad_content.checksum)
                if not is_valid:
                    all_checksums_verified = False
                    message = f"Checksum mismatch for file with id {ad_content.file_id}."
                    status = ResultStatus.FAILED
                    break

                total_downloaded_so_far += ad_content.size
            else:  # for-else loop: executed if the loop finished without break.
                if all_checksums_verified:
                    status = ResultStatus.SUCCESS
                    message = "All advertisement contents downloaded successfully."
                    # 메트릭 수집기에 광고 목록 업데이트
                    ad_ids = [{ "id": content.file_id } for content in download_request.contents]
                    self._metrics_collector.update_advertisements(ad_ids)

        except TimeoutError as e:
            status = ResultStatus.TIMEOUT
            message = str(e)
        except Exception as e:
            status = ResultStatus.FAILED
            message = f"An unexpected error occurred: {e}"
        finally:
            download_ms = int((time.time() - start_time) * 1000)
            self._send_result(
                download_request.command_id,
                status,
                message,
                all_checksums_verified,
                download_ms,
            )
