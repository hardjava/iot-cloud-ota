// 전체 시스템의 주요 컴포넌트를 초기화하고 연결하는 역할을 합니다.
package cmd

import (
	"context"
	"mqtt-handler/config"
	"mqtt-handler/mqttclient"
	"mqtt-handler/network"
	"mqtt-handler/repository"
)

// 애플리케이션의 핵심 구성 요소들을 담고 있는 컨테이너입니다.
// config: 환경 변수로부터 로드된 설정 정보를 보관
// network: HTTP 서버와 관련된 기능들을 관리
type Cmd struct {
	config        *config.Config
	network       *network.Network
	mqttClient    *mqttclient.MQTTClient
	questDBClient *repository.DBClient
}

// Cmd 구조체를 초기화하고, HTTP 서버를 시작합니다.
func NewCmd() *Cmd {
	c := &Cmd{
		config:     config.NewConfig(),
		network:    network.NewNetwork(),
		mqttClient: mqttclient.NewMqttClient(),
	}
	ctx := context.TODO()

	// Consumer별로 독립 DBClient 생성
	go repository.NewDBClient(ctx, c.config.QuestDB.Conf).StartEventConsumer()
	go repository.NewDBClient(ctx, c.config.QuestDB.Conf).StartSystemStatusConsumer()
	go repository.NewDBClient(ctx, c.config.QuestDB.Conf).StartErrorLogConsumer()
	go repository.NewDBClient(ctx, c.config.QuestDB.Conf).StartSalesItemConsumer()

	// MQTT 연결
	c.mqttClient.Connect(c.config.MqttBroker.Url, c.config.MqttBroker.ClientId)

	// HTTP 서버 시작
	if err := c.network.ServerStart(c.config.Server.Port); err != nil {
		panic(err)
	}
	return c
}
