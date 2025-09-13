package repository

import (
	"context"
	"github.com/questdb/go-questdb-client/v3"
	"log"
	"mqtt-handler/types"
)

// 다운로드 이벤트 처리를 위한 비동기 채널
var DownloadInsertChan = make(chan types.DownloadEvent, 1000000)

// 기기 상태 이벤트 처리를 위한 비동기 채널
var SystemStatusInsertChan = make(chan types.SystemStatusEvent, 10000)

// 에러 로그 이벤트 처리를 위한 비동기 채널
var ErrorLogInsertChan = make(chan types.ErrorLogEvent, 10000)

// 판매 데이터 이벤트 처리를 위한 비동기 채널 10000개
var SalesDataInsertChan = make(chan types.SalesDataEvent, 10000)

// DBClient는 QuestDB와 통신하기 위한 객체입니다.
type DBClient struct {
	sender questdb.LineSender
}

// DBClient 생성자
func NewDBClient(ctx context.Context, confStr string) *DBClient {
	sender, err := questdb.LineSenderFromConf(ctx, confStr)
	if err != nil {
		log.Fatalf("[QuestDB] 연결 실패: %v", err)
	}
	log.Println("[QuestDB] 연결 완료")
	return &DBClient{sender: sender}
}
