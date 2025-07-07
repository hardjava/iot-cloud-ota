# NOTE: EMQX는 ELB를 사용하여 클러스터를 구성하고 부하 분산을 하는 것이 좋습니다.
# 하지만 현재 개발 환경에서는 비용적인 한계로 단일 인스턴스로 EMQX를 배포합니다.
# 이후 프로덕션 환경에서는 ELB를 사용하여 EMQX 클러스터를 구성하는 것을 고려합니다.

resource "aws_instance" "emqx" {
  ami                    = "ami-0c9c942bd7bf113a2" # Ubuntu 22.04 LTS
  instance_type          = "t3.micro"
  vpc_security_group_ids = [aws_security_group.emqx_sg.id]
  subnet_id              = aws_subnet.public_a.id

  user_data = <<-EOF
    #!/bin/bash
    sudo apt-get update && \
    curl -s https://assets.emqx.com/scripts/install-emqx-deb.sh | sudo bash && \
    sudo apt-get install -y emqx && \
    sudo systemctl start emqx
  EOF

  tags = {
    Name        = "iot-cloud-ota-emqx"
    Description = "EMQX broker for iot-cloud-ota"
  }
}
