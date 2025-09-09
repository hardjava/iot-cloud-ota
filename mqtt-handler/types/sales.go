package types

import "time"

type SalesData struct {
	Type      string    `json:"type"`
	SubType   string    `json:"sub_type"`
	Timestamp time.Time `json:"timestamp"`
}

type SalesDataEvent struct {
	DeviceId  int64
	SalesData SalesData
}
