package repository

import (
	"context"
	"log"
	"time"
)

// StartEventConsumer : download_events
func (c *DBClient) StartEventConsumer() {
	ticker := time.NewTicker(1 * time.Second) // 1초마다 flush
	defer ticker.Stop()

	for {
		select {
		case event := <-DownloadInsertChan:
			ctx := context.TODO()
			if err := c.sender.Table("download_events").
				Symbol("command_id", event.CommandID).
				Symbol("message", event.Message).
				Symbol("status", event.Status).
				Int64Column("device_id", event.DeviceID).
				Int64Column("progress", event.Progress).
				Int64Column("total_bytes", event.TotalBytes).
				Int64Column("download_bytes", event.DownloadBytes).
				Float64Column("speed_kbps", event.SpeedKbps).
				BoolColumn("checksum_verified", event.ChecksumVerified).
				Int64Column("download_ms", event.DownloadMs).
				At(ctx, event.Timestamp.UTC()); err != nil {
				log.Printf("[ERROR] download_events insert 실패: %v", err)
			}

		case <-ticker.C:
			if err := c.sender.Flush(context.TODO()); err != nil {
				log.Printf("[ERROR] QuestDB Flush 실패: %v", err)
			}
		}
	}
}

// StartSystemStatusConsumer : system_status, network_status, advertisement_status, firmware_status
func (c *DBClient) StartSystemStatusConsumer() {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for {
		select {
		case event := <-SystemStatusInsertChan:
			ctx := context.TODO()

			if err := c.sender.Table("system_status").
				Int64Column("device_id", event.DeviceId).
				Float64Column("cpu_core_0", event.System.CpuUsage.Core0).
				Float64Column("cpu_core_1", event.System.CpuUsage.Core1).
				Float64Column("memory_usage", event.System.MemoryUsage).
				Float64Column("storage_usage", event.System.StorageUsage).
				Int64Column("uptime", event.System.Uptime).
				At(ctx, event.Timestamp.UTC()); err != nil {
				log.Printf("[ERROR] system_status insert 실패: %v", err)
			}

			if err := c.sender.Table("network_status").
				Int64Column("device_id", event.DeviceId).
				StringColumn("connection_type", event.Network.ConnectionType).
				Int64Column("signal_strength", event.Network.SignalStrength).
				StringColumn("local_ip", event.Network.LocalIp).
				StringColumn("gateway_ip", event.Network.GatewayIp).
				At(ctx, event.Timestamp.UTC()); err != nil {
				log.Printf("[ERROR] network_status insert 실패: %v", err)
			}

			for _, ad := range event.Advertisements {
				if err := c.sender.Table("advertisement_status").
					Int64Column("device_id", event.DeviceId).
					Int64Column("ad_id", ad.Id).
					At(ctx, event.Timestamp.UTC()); err != nil {
					log.Printf("[ERROR] advertisement_status insert 실패: %v", err)
				}
			}

			if err := c.sender.Table("firmware_status").
				Int64Column("device_id", event.DeviceId).
				StringColumn("firmware_version", event.FirmwareVersion).
				At(ctx, event.Timestamp.UTC()); err != nil {
				log.Printf("[ERROR] firmware_status insert 실패: %v", err)
			}

		case <-ticker.C:
			if err := c.sender.Flush(context.TODO()); err != nil {
				log.Printf("[ERROR] QuestDB Flush 실패: %v", err)
			}
		}
	}
}

// StartErrorLogConsumer : error_logs
func (c *DBClient) StartErrorLogConsumer() {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for {
		select {
		case event := <-ErrorLogInsertChan:
			ctx := context.TODO()
			if err := c.sender.Table("error_logs").
				Int64Column("device_id", event.DeviceId).
				StringColumn("error_tag", event.ErrorLog.ErrorTag).
				StringColumn("log", event.ErrorLog.Log).
				At(ctx, event.ErrorLog.Timestamp.UTC()); err != nil {
				log.Printf("[ERROR] error_logs insert 실패: %v", err)
			}

		case <-ticker.C:
			if err := c.sender.Flush(context.TODO()); err != nil {
				log.Printf("[ERROR] QuestDB Flush 실패: %v", err)
			}
		}
	}
}

// StartSalesItemConsumer : sales_data
func (c *DBClient) StartSalesItemConsumer() {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for {
		select {
		case event := <-SalesDataInsertChan:
			ctx := context.TODO()
			if err := c.sender.Table("sales_data").
				Int64Column("device_id", event.DeviceId).
				StringColumn("type", event.SalesData.Type).
				StringColumn("sub_type", event.SalesData.SubType).
				At(ctx, event.SalesData.Timestamp.UTC()); err != nil {
				log.Printf("[ERROR] sales_data insert 실패: %v", err)
			}

		case <-ticker.C:
			if err := c.sender.Flush(context.TODO()); err != nil {
				log.Printf("[ERROR] QuestDB Flush 실패: %v", err)
			}
		}
	}
}
