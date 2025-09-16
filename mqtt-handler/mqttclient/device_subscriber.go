package mqttclient

import (
	"bytes"
	"encoding/json"
	"fmt"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"io"
	"log"
	"mqtt-handler/config"
	"mqtt-handler/types"
	"net/http"
	"strconv"
	"strings"
)

var httpClient = &http.Client{}

// subscribeDeviceRegister는 디바이스 등록 요청을 구독하고 처리합니다.
func (m *MQTTClient) subscribeDeviceRegister(topic string, label string) {
	handler := func(client mqtt.Client, msg mqtt.Message) {

		var req types.DeviceRegisterRequest
		if err := json.Unmarshal(msg.Payload(), &req); err != nil {
			log.Printf("[ERROR] %s 메시지 파싱 실패: %v", label, err)
			return
		}

		topicParts := strings.Split(msg.Topic(), "/")
		deviceId, _ := strconv.ParseInt(topicParts[1], 10, 64)

		req.DeviceId = deviceId

		go forwardToSpring(&req, m)
	}

	token := m.mqttClient.Subscribe(topic, 1, handler)
	token.Wait()
	if token.Error() != nil {
		log.Printf("[ERROR] %s 구독 실패: %v", label, token.Error())
	} else {
		log.Printf("[MQTT] 구독 성공: %s", topic)
	}
}

// Spring 서버로 디바이스 등록 요청을 전달합니다.
func forwardToSpring(req *types.DeviceRegisterRequest, m *MQTTClient) {
	url := config.SpringUrl + "/api/devices/register"
	jsonData, _ := json.Marshal(req)

	resp, err := httpClient.Post(url, "application/json", bytes.NewBuffer(jsonData))
	if err != nil {
		log.Printf("[ERROR] Spring 전송 실패 (deviceId=%d): %v", req.DeviceId, err)
		return
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)

	var springResp types.DeviceRegisterResponse
	if err := json.Unmarshal(body, &springResp); err != nil {
		log.Printf("[ERROR] 응답 파싱 실패: %v", err)
		return
	}
	
	ackTopic := fmt.Sprintf("v1/%d/regist/ack", req.DeviceId)
	ackPayload, _ := json.Marshal(springResp)

	if token := m.mqttClient.Publish(ackTopic, 1, false, ackPayload); token.Wait() && token.Error() != nil {
		log.Printf("[ERROR] MQTT publish 실패 (topic=%s, deviceId=%d): %v", ackTopic, req.DeviceId, token.Error())
	}
}
