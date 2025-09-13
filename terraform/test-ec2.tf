resource "aws_instance" "test_instance" {
  count = 1

  ami                         = "ami-08fbd9515ac39abc3"
  instance_type               = "t4g.xlarge"
  subnet_id                   = aws_subnet.public_a.id
  vpc_security_group_ids      = [aws_security_group.bastion_sg.id]
  associate_public_ip_address = true
  key_name                    = data.aws_key_pair.ssh_key.key_name

  tags = {
    Name = "test-instance-${count.index + 1}"
  }
}
