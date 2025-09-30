# IoT 클라우드 OTA(Over-the-Air) 업데이트 인프라 보고서

## 1. 개요

이 문서는 대규모 IoT 기기를 위한 OTA(Over-the-Air) 업데이트 인프라의 아키텍처와 주요 특징을 상세히 기술한 보고서이다. 본 인프라는 AWS(Amazon Web Services) 환경에 Terraform을 사용하여 코드형 인프라(Infrastructure as Code, IaC)로 구축되었다.

프로젝트의 핵심 목표는 수많은 IoT 기기에 펌웨어 및 콘텐츠를 안정적이고, 안전하며, 확장 가능하게 배포하는 것이다. 이를 위해 **고가용성**, **강화된 보안**, 그리고 **효율적인 데이터 처리**라는 세 가지 핵심 원칙에 중점을 두고 아키텍처를 설계했다.

---

## 2. 고가용성 및 확장성 구현

대규모 IoT 트래픽을 안정적으로 처리하고, 장애 발생 시에도 서비스 연속성을 보장하기 위해 다음과 같은 기술과 전략을 채택했다.

### 컨테이너 기반의 유연한 서비스 운영

서버 프로비저닝 및 관리에 드는 운영 부담을 줄이고 애플리케이션 개발과 배포에 집중하기 위해, 주요 서비스를 컨테이너화하고 **Amazon ECS (Elastic Container Service)** 에서 실행하도록 결정했다. 특히, 서버리스 컨테이너 엔진인 **Fargate**를 시작 유형으로 선택하여 EC2 인스턴스 클러스터를 직접 관리할 필요가 없도록 구성했다. 이를 통해 트래픽 변화에 따른 확장 및 축소가 용이해지고, 인프라 관리의 복잡성을 크게 낮추는 장점을 얻을 수 있었다.

모든 ECS 서비스는 아래 코드와 같이 `FARGATE` 시작 유형으로 정의되었으며, 가용성을 위해 여러 가용 영역에 걸쳐있는 Private Subnet에 배포되었다.

```terraform
# backend.tf - 백엔드 서비스 정의 예시
resource "aws_ecs_service" "backend" {
  name            = "backend"
  cluster         = aws_ecs_cluster.main.id
  task_definition = aws_ecs_task_definition.backend.arn
  desired_count   = 1
  launch_type     = "FARGATE"

  network_configuration {
    subnets          = [aws_subnet.private_a.id, aws_subnet.private_b.id]
    security_groups  = [aws_security_group.backend.id, aws_security_group.ecs_internal_services.id]
    assign_public_ip = false
  }
  # ...
}
```

### 대규모 MQTT 트래픽 처리를 위한 아키텍처

수십만 대 이상의 IoT 기기에서 발생하는 대규모 동시 연결 및 메시지 트래픽을 처리하는 것은 이 프로젝트의 핵심 요구사항이었다. 이를 해결하기 위해 고성능 MQTT 브로커인 **EMQX**를 ECS에 배포하고, 그 앞단에 **Network Load Balancer (NLB)** 를 배치하는 아키텍처를 선택했다.

HTTP/HTTPS 트래픽에 최적화된 Application Load Balancer(ALB)와 달리, NLB는 TCP/UDP 트래픽을 처리하는 Layer 4 로드 밸런서로, 대량의 영구적인 TCP 연결을 유지해야 하는 MQTT 프로토콜에 훨씬 적합하다. NLB를 사용함으로써 연결 상태를 유지하면서 높은 처리량과 낮은 지연 시간을 달성할 수 있었고, 이는 대규모 IoT 환경의 안정성을 확보하는 데 결정적인 역할을 했다.

```terraform
# emqx.tf - MQTT 트래픽을 위한 NLB 및 리스너 정의
resource "aws_lb" "emqx_nlb" {
  name               = "emqx-nlb"
  internal           = false
  load_balancer_type = "network"
  subnets            = [aws_subnet.public_a.id, aws_subnet.public_b.id]
}

resource "aws_lb_listener" "nlb_listener_mqtt_tls" {
  load_balancer_arn = aws_lb.emqx_nlb.arn
  port              = 8883 # mTLS
  protocol          = "TCP"

  default_action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.nlb_tg_mqtt_tls.arn
  }
}
```

---

## 3. 시계열 데이터 처리를 위한 QuestDB 도입

IoT 환경에서는 기기의 상태, 센서 값, 펌웨어 업데이트 이력 등 시간의 흐름에 따라 대량으로 발생하는 데이터, 즉 **시계열 데이터(Time-Series Data)** 가 주를 이룬다. 이러한 데이터를 효율적으로 처리하기 위해 범용적인 RDBMS(관계형 데이터베이스) 대신 시계열 데이터 처리에 특화된 **QuestDB**를 도입했다.

### QuestDB 도입 이유

펌웨어 배포 이력이나 기기의 주기적인 상태 보고와 같은 데이터는 '추가'는 매우 빈번하지만 '수정'이나 '삭제'는 거의 발생하지 않는 특성을 가진다. RDBMS에 이러한 데이터를 대량으로 저장할 경우, 시간이 지남에 따라 인덱싱과 조회 성능이 저하될 수 있다. 반면, QuestDB와 같은 시계열 데이터베이스(TSDB)는 빠른 속도로 데이터를 수집(Ingestion)하고, 시간 범위에 기반한 복잡한 집계 및 분석 쿼리를 매우 효율적으로 수행하도록 설계되었다. QuestDB를 사용함으로써 대규모 기기로부터 수집되는 데이터를 실시간으로 저장하고, Grafana를 통해 즉각적으로 시각화 및 분석할 수 있는 강력한 모니터링 시스템의 기반을 마련할 수 있었다.

### EBS 볼륨을 통한 데이터 영속성 확보

QuestDB는 EC2 인스턴스 위에서 Docker 컨테이너로 실행된다. 이때 발생할 수 있는 문제는 EC2 인스턴스가 장애로 종료되거나 교체될 경우 컨테이너 내부에 저장된 모든 데이터가 유실될 수 있다는 점이다. 이러한 문제를 해결하고 데이터의 영속성을 보장하기 위해, 데이터의 저장 위치를 EC2 인스턴스로부터 분리하는 전략을 사용했다.

EC2 인스턴스와는 별개의 생명주기를 가지는 **EBS(Elastic Block Store) 볼륨**을 생성하고, 이를 QuestDB를 실행할 EC2 인스턴스에 연결(Attach)했다. 그리고 QuestDB 컨테이너를 실행할 때, 데이터가 저장되는 경로를 이 EBS 볼륨의 마운트 경로로 지정했다. 이 방식을 통해 EC2 인스턴스가 교체되더라도, 새로운 인스턴스에 기존 EBS 볼륨을 다시 연결하기만 하면 모든 과거 데이터를 그대로 보존할 수 있게 되었다. 이는 데이터의 안정성과 내구성을 확보하는 핵심적인 설계이다.

```terraform
# quest_db.tf - EBS 볼륨 생성 및 연결

# 1. EC2 인스턴스와 독립적인 EBS 볼륨을 생성
resource "aws_ebs_volume" "questdb_volume" {
  availability_zone = aws_subnet.private_a.availability_zone
  size              = 20
  type              = "gp3"
  tags = { Name = "questdb-volume" }
}

# 2. 생성된 EBS 볼륨을 QuestDB용 EC2 인스턴스에 연결
resource "aws_volume_attachment" "questdb_attachment" {
  device_name = "/dev/xvdb"
  volume_id   = aws_ebs_volume.questdb_volume.id
  instance_id = aws_instance.questdb.id
}

# 3. EC2 인스턴스 사용자 데이터(user_data)에서 볼륨을 마운트하고
#    Docker 컨테이너의 데이터 경로로 사용
resource "aws_instance" "questdb" {
  # ...
  user_data = <<-"EOF"
    #!/bin/bash
    # ... (볼륨 포맷 및 마운트)
    mkdir -p /data
    mount /dev/nvme1n1 /data

    # ... (도커 설치)

    # QuestDB 컨테이너 실행 시, 호스트의 /data 경로(EBS 볼륨)를
    # 컨테이너의 데이터 저장 경로인 /var/lib/questdb로 마운트
    docker run -d --name questdb \
      -p 9000:9000 -p 8812:8812 \
      -v /data:/var/lib/questdb \
      questdb/questdb:9.0.0
  EOF
}
```

---

## 4. 강화된 보안 아키텍처 구현

IoT 환경에서는 기기, 네트워크, 클라우드에 이르는 모든 계층에서 강력한 보안이 필수적이다. 본 프로젝트는 외부 공격으로부터 내부 시스템을 보호하고, 허가된 기기만이 안전하게 통신할 수 있도록 다층적인 보안 전략을 적용했다.

### Private Subnet 중심의 네트워크 격리

보안 설계의 가장 기본 원칙은 공격 표면(Attack Surface)을 최소화하는 것이다. 이를 위해 인터넷에서 직접 접근할 필요가 없는 모든 핵심 리소스들을 **Private Subnet**에 배치했다. 데이터베이스(RDS), 캐시(ElastiCache), 그리고 모든 ECS 서비스들이 여기에 해당된다. 외부와의 통신은 반드시 필요한 경우에만 Public Subnet에 위치한 로드 밸런서(ALB, NLB)나 NAT 인스턴스를 통해서만 이루어지도록 설계하여, 내부 시스템을 외부 위협으로부터 원천적으로 격리했다. 이 구조는 인프라 보안의 가장 중요한 첫 번째 방어선 역할을 했다.

```terraform
# vpc.tf - Public/Private 서브넷 정의
resource "aws_subnet" "public_a" {
  vpc_id                  = aws_vpc.main.id
  cidr_block              = "10.0.10.0/24"
  availability_zone       = "ap-northeast-2a"
  map_public_ip_on_launch = true # Public IP 자동 할당
}

resource "aws_subnet" "private_a" {
  vpc_id            = aws_vpc.main.id
  cidr_block        = "10.0.0.0/24"
  availability_zone = "ap-northeast-2a"
  # map_public_ip_on_launch는 기본적으로 false
}
```

### 사설 CA를 이용한 상호 인증(mTLS)

단순히 통신을 암호화하는 것을 넘어, 서버와 클라이언트가 서로의 신원을 확인하는 강력한 인증 체계를 구축하고자 했다. 특히, 허가된 IoT 기기만이 우리 시스템에 접속할 수 있도록 보장하는 것이 중요했다. 이를 위해 서버만 인증하는 단방향 TLS가 아닌, 기기(클라이언트)와 서버가 상호 인증하는 **mTLS(Mutual TLS)** 방식을 도입했다.

이를 구현하기 위해 ECS 컨테이너 기반의 **사설 인증 기관(Private CA)** 서비스를 직접 구축했다. 이 CA는 각 IoT 기기에 배포될 고유한 클라이언트 인증서를 발급하는 역할을 한다. 기기는 이 인증서가 있어야만 MQTT 브로커(EMQX)에 접속을 시도할 수 있으며, 브로커는 이 인증서가 우리가 발급한 유효한 인증서인지 검증한다. 이 방식을 통해 인증되지 않은 기기의 접속을 원천 차단하고, 중간자 공격(Man-in-the-Middle)과 같은 심각한 보안 위협을 효과적으로 방어할 수 있었다.

### 최소 권한 원칙 기반의 Security Group

네트워크 격리만으로는 내부망에서의 위협에 취약할 수 있다. 따라서 각 서비스(리소스)가 통신할 수 있는 대상을 엄격하게 제한하는 **최소 권한의 원칙**을 적용했다. AWS의 Security Group을 사용하여, 각 서비스는 명시적으로 허용된 대상하고만, 그리고 꼭 필요한 포트와 프로토콜로만 통신할 수 있도록 설정했다.

예를 들어, 아래 코드는 데이터베이스(MySQL)의 보안 그룹 설정이다. `ingress` 규칙을 보면, 오직 백엔드 서비스(`aws_security_group.backend.id`)와 관리용 Bastion Host(`aws_security_group.bastion_sg.id`) 등 허가된 보안 그룹으로부터의 3306 포트 TCP 요청만을 허용하고 있다. 만약 다른 서비스가 해킹되더라도 데이터베이스에 직접 접근하는 2차 피해를 막을 수 있는 중요한 보안 장치이다.

```terraform
# security_groups.tf - RDS(MySQL) 보안 그룹 정의
resource "aws_security_group" "mysql_sg" {
  name   = "iot-cloud-ota-mysql-sg"
  vpc_id = aws_vpc.main.id

  ingress {
    from_port = 3306
    to_port   = 3306
    protocol  = "tcp"
    security_groups = [
      aws_security_group.lambda_sg.id,
      aws_security_group.bastion_sg.id,
      aws_security_group.backend.id,
    ]
    description = "MySQL access from trusted security groups"
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}
```

### CloudFront Signed URL을 통한 안전한 펌웨어 배포

펌웨어 파일과 같은 민감한 콘텐츠를 아무나 다운로드할 수 없도록 보호하는 것은 필수적이다. S3 버킷을 공개로 설정하는 대신, **CloudFront Signed URL**을 사용하는 방식을 채택했다. 펌웨어 파일이 저장된 S3 버킷은 비공개로 유지하고, CloudFront를 통해서만 접근할 수 있도록 설정했다. 이때, CloudFront 배포 설정에 `trusted_key_groups`를 지정하여 서명된 요청만을 신뢰하도록 구성했다.

기기가 펌웨어 업데이트를 요청하면, 백엔드 서비스는 이 신뢰 키 그룹의 비공개 키를 사용하여 짧은 만료 시간을 가진 Signed URL을 동적으로 생성하여 기기에 전달한다. 이 URL을 통해서만 펌웨어 다운로드가 가능하므로, 허가되지 않은 사용자의 파일 접근을 막고 콘텐츠를 안전하게 보호하는 효과를 얻었다.

```terraform
# buckets.tf - 서명된 URL을 요구하는 CloudFront 배포 설정
resource "aws_cloudfront_distribution" "firmware_distribution" {
  # ...
  default_cache_behavior {
    # ...
    viewer_protocol_policy = "redirect-to-https"
    trusted_key_groups     = [aws_cloudfront_key_group.signing_key_group.id]
  }
  # ...
}

resource "aws_cloudfront_key_group" "signing_key_group" {
  name  = "cloudfront-signing-key-group"
  items = [aws_cloudfront_public_key.signing_key.id]
}
```

---

## 5. 결론

본 IoT OTA 업데이트 인프라는 AWS의 관리형 서비스와 컨테이너 기술을 적극적으로 활용하여 **높은 가용성**과 **유연한 확장성**을 확보했다. 또한 시계열 데이터베이스를 도입하여 **효율적인 데이터 처리 및 분석**의 기틀을 마련했다. 동시에 VPC, mTLS, Security Group, Signed URL 등 구체적이고 강력한 다층적 보안 장치를 적용하여 **견고한 보안 체계**를 구축했다. Terraform을 통해 모든 인프라를 코드로 관리함으로써, 인프라의 일관성을 유지하고 변경 사항을 추적하며, 신속하고 안정적인 배포가 가능하도록 설계되었다.

---

## 6. CI/CD(지속적 통합 및 배포) 파이프라인

본 프로젝트는 여러 마이크로서비스가 하나의 리포지토리에서 관리되는 **모노레포(Monorepo)** 구조를 채택하고 있다. 모든 서비스는 Docker 컨테이너로 빌드되어 Amazon ECS에서 실행되므로, 코드 변경 사항을 안정적이고 신속하게 통합하고 배포하는 자동화된 파이프라인은 필수적이다. 수동 배포는 실수를 유발하기 쉽고, 서비스 간의 의존성 관리를 복잡하게 만들어 전체 개발 및 운영 효율성을 저하시키기 때문이다.

이를 해결하기 위해, GitHub Actions를 활용하여 `main` 브랜치에 코드가 푸시될 때마다 자동으로 빌드와 배포가 이루어지는 CI/CD 파이프라인을 구축했다.

### 파이프라인 핵심 동작 방식

파이프라인은 효율성과 안정성에 초점을 맞춰 다음과 같이 동작한다.

1.  **변경 감지 (Change Detection)**: 파이프라인이 시작되면, 가장 먼저 어떤 서비스의 코드가 변경되었는지 자동으로 감지한다. 이는 `dorny/paths-filter` 액션을 사용하여 구현되었다. 이 단계를 통해 변경되지 않은 서비스는 불필요하게 재빌드 및 재배포되지 않으므로, 리소스를 절약하고 배포 시간을 크게 단축한다.

    ```yaml
    # .github/workflows/ci_dev.yml - 변경 감지 부분
    - uses: dorny/paths-filter@v2
      id: filter
      with:
        filters: |
          backend:
            - 'backend/**'
          emqx:
            - 'emqx/**'
          # ... (기타 서비스)
    ```

2.  **병렬 빌드 및 ECR 푸시 (Parallel Build & Push)**: 변경이 감지된 각 서비스에 대해, 독립적인 빌드 작업이 **병렬로** 실행된다. 각 작업은 해당 서비스의 `Dockerfile`을 사용하여 Docker 이미지를 빌드하고, 고유한 식별을 위해 Git 커밋 SHA를 태그로 붙여 Amazon ECR(Elastic Container Registry)에 푸시한다. 이 방식은 전체 빌드 시간을 최소화하고, 어떤 코드가 어떤 이미지에 해당하는지 명확하게 추적할 수 있게 해준다.

3.  **Terraform을 이용한 자동 배포 (Terraform Apply)**: 모든 빌드 작업이 성공적으로 완료되면, 마지막 배포 단계가 실행된다. 이 단계에서는 Terraform을 사용하여 인프라 변경 사항을 적용한다. 특히, 이전 단계에서 ECR에 푸시된 새로운 이미지의 태그(Git 커밋 SHA)를 변수로 받아 `terraform apply` 명령을 실행한다.

    ```yaml
    # .github/workflows/ci_dev.yml - Terraform 배포 부분
    - name: Terraform Apply
      env:
        BACKEND_IMAGE: ${{ needs.ci-backend-dev.outputs.tag }}
        # ... (기타 서비스 이미지 태그)
      run: |
        image_tags=""
        if [ -n "$BACKEND_IMAGE" ]; then
          image_tags="$image_tags -var backend_image_tag=$BACKEND_IMAGE"
        fi
        # ... (변경된 다른 이미지에 대한 변수 추가)

        terraform apply -auto-approve $image_tags
    ```

    이때, 파이프라인은 변경된 서비스의 이미지 태그만 `terraform apply` 명령의 변수로 전달한다. 결과적으로 Terraform은 해당 ECS 서비스의 작업 정의(Task Definition)만 새로운 이미지 버전으로 업데이트하여, 효율적이고 정확한 배포를 수행한다.

이러한 자동화된 CI/CD 파이프라인을 통해 개발자는 코드 작성에만 집중할 수 있으며, 인프라는 항상 최신 상태의 코드를 안정적으로 반영하게 된다. 이는 전체 시스템의 신뢰성을 높이고 빠른 기능 반복(Iteration)을 가능하게 하는 핵심 요소이다.