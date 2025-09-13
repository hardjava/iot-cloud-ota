package mqttclient

import (
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"mqtt-handler/repository"
	"mqtt-handler/types"
	"time"
)

const publishDelay = 10 * time.Millisecond

// MQTT 클라이언트를 사용해 지정된 토픽으로 JSON 형태의 펌웨어 배포 요청 메시지를 전송합니다.
func (m *MQTTClient) PublishDownloadRequest(req *types.FirmwareDeployRequest) {
	var buf bytes.Buffer

	command := types.FirmwareDownloadCommand{
		CommandID: req.CommandId,
		Content:   req.Content,
		Timestamp: req.Timestamp,
	}

	enc := json.NewEncoder(&buf)
	enc.SetEscapeHTML(false)
	if err := enc.Encode(command); err != nil {
		log.Printf("[ERROR] JSON 직렬화 실패: %v", err)
		return
	}
	payload := buf.String()

	go func() {
		for _, deviceInfo := range req.Devices {
			topic := fmt.Sprintf("v1/%d/update/request/firmware", deviceInfo.DeviceId)
			token := m.mqttClient.Publish(topic, 1, false, payload)

			event := types.DownloadEvent{
				CommandID:        command.CommandID,
				DeviceID:         deviceInfo.DeviceId,
				Message:          "Download Command",
				Status:           "WAITING",
				Progress:         0,
				TotalBytes:       command.Content.FileInfo.Size,
				DownloadBytes:    0,
				SpeedKbps:        0,
				ChecksumVerified: false,
				DownloadMs:       0,
			}

			repository.DownloadInsertChan <- event
			token.Wait()
			if token.Error() != nil {
				log.Printf("[MQTT] Publish 실패: %s → %v", topic, token.Error())
			}
			time.Sleep(publishDelay)
		}
	}()
}

// MQTT 클라이언트를 사용해 지정된 토픽으로 JSON 형태의 펌웨어 배포 취소 요청 메시지를 전송합니다.
func (m *MQTTClient) PublishDownloadCancelRequest(req *types.DeployCancelRequest) {
	command := types.DownloadCancelCommand{
		CommandID: req.CommandID,
		Reason:    req.Reason,
		Timestamp: req.Timestamp,
	}

	payload, err := json.Marshal(command)
	if err != nil {
		log.Printf("[ERROR] JSON 직렬화 실패: %v", err)
		return
	}

	go func() {
		for _, deviceInfo := range req.Devices {
			topic := fmt.Sprintf("v1/%d/update/cancel", deviceInfo.DeviceId)
			token := m.mqttClient.Publish(topic, 2, false, payload)

			event := types.DownloadEvent{
				CommandID:        command.CommandID,
				DeviceID:         deviceInfo.DeviceId,
				Message:          command.Reason,
				Status:           "CANCELED",
				Progress:         0,
				TotalBytes:       0,
				DownloadBytes:    0,
				SpeedKbps:        0,
				ChecksumVerified: false,
				DownloadMs:       0,
			}

			repository.DownloadInsertChan <- event
			token.Wait()
			if token.Error() != nil {
				log.Printf("[MQTT] Publish 실패: %s → %v", topic, token.Error())
			}
			time.Sleep(publishDelay)
		}
	}()
}

func (m *MQTTClient) PublishAdsDownloadRequest(req *types.AdsDeployRequest) {
	var buf bytes.Buffer

	command := types.AdsDownloadCommand{
		CommandID: req.CommandId,
		Contents:  req.Contents,
		Timestamp: req.Timestamp,
	}

	totalSize := 0
	for _, content := range req.Contents {
		totalSize += int(content.FileInfo.Size)
	}

	enc := json.NewEncoder(&buf)
	enc.SetEscapeHTML(false)
	if err := enc.Encode(command); err != nil {
		log.Printf("[ERROR] JSON 직렬화 실패: %v", err)
		return
	}
	payload := buf.String()

	go func() {
		for _, deviceInfo := range req.Devices {
			topic := fmt.Sprintf("v1/%d/update/request/advertisement", deviceInfo.DeviceId)
			token := m.mqttClient.Publish(topic, 2, false, payload)

			event := types.DownloadEvent{
				CommandID:        command.CommandID,
				DeviceID:         deviceInfo.DeviceId,
				Message:          "Download Command",
				Status:           "WAITING",
				Progress:         0,
				TotalBytes:       int64(totalSize),
				DownloadBytes:    0,
				SpeedKbps:        0,
				ChecksumVerified: false,
				DownloadMs:       0,
			}

			repository.DownloadInsertChan <- event
			token.Wait()
			if token.Error() != nil {
				log.Printf("[MQTT] Publish 실패: %s → %v", topic, token.Error())
			}
			time.Sleep(publishDelay)
		}
	}()
}
