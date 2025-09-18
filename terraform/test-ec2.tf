# NOTE: 시뮬레이터를 실행할 EC2 인스턴스를 생성합니다. 비용 절감을 위해 count를 0으로 설정해두었습니다.
resource "aws_instance" "test_instance" {
  count = 0

  ami                         = "ami-08fbd9515ac39abc3"
  instance_type               = "t4g.xlarge"
  subnet_id                   = aws_subnet.public_a.id
  vpc_security_group_ids      = [aws_security_group.bastion_sg.id]
  associate_public_ip_address = true
  key_name                    = data.aws_key_pair.ssh_key.key_name

  user_data = <<-EOF
    #!/bin/bash
    
    sudo yum install git -y
    
    git clone https://github.com/coffee-is-essential/iot-cloud-ota.git

    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py

    python3 get-pip.py

    pip3 install -r iot-cloud-ota/device_simulator/requirements.txt
  EOF

  tags = {
    Name = "test-instance-${count.index + 1}"
  }
}
