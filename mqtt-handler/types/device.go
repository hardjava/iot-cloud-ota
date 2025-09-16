package types

import "time"

type DeviceRegisterRequest struct {
	DeviceId   int64     `json:"device_id"`
	DeviceName string    `json:"device_name"`
	AuthKey    string    `json:"auth_key"`
	Timestamp  time.Time `json:"timestamp"`
}

type DeviceRegisterResponse struct {
	Status    string    `json:"status"`
	Timestamp time.Time `json:"timestamp"`
}
