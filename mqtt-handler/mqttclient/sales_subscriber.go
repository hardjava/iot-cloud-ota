package mqttclient

import (
	"encoding/json"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"log"
	"mqtt-handler/repository"
	"mqtt-handler/types"
	"strconv"
	"strings"
)

// 에러 로그 메시지 구독 및 처리
func (m *MQTTClient) subscribeSalesData(topic string, parseFunc func(mqtt.Message) (*types.SalesDataEvent, error), label string) {
	handler := func(client mqtt.Client, msg mqtt.Message) {
		log.Printf("[MQTT] %s 수신 - 토픽: %s", label, msg.Topic())

		event, err := parseFunc(msg)
		if err != nil {
			log.Printf("[ERROR] %s 메시지 파싱 실패: %v", label, err)
			return
		}

		repository.SalesDataInsertChan <- *event
	}

	token := m.mqttClient.Subscribe(topic, 1, handler)
	token.Wait()
	if token.Error() != nil {
		log.Printf("[ERROR] %s 구독 실패: %v", label, token.Error())
	} else {
		log.Printf("[MQTT] 구독 성공: %s", topic)
	}
}

// 에러 로그 메시지 파싱
func parseSalesItem(msg mqtt.Message) (*types.SalesDataEvent, error) {
	var salesData types.SalesData
	if err := json.Unmarshal(msg.Payload(), &salesData); err != nil {

		return nil, err
	}

	return buildSalesItem(msg.Topic(), salesData), nil
}

// 이벤트 빌더 - 토픽에서 ID 추출 + 이벤트 생성
func buildSalesItem(topic string, salesData types.SalesData) *types.SalesDataEvent {
	topicParts := strings.Split(topic, "/")
	deviceId, _ := strconv.ParseInt(topicParts[1], 10, 64)

	return &types.SalesDataEvent{
		DeviceId:  deviceId,
		SalesData: salesData,
	}
}
