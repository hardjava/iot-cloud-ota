resource "aws_s3_bucket" "this" {
  bucket = var.bucket_name

  tags = {
    environment = var.environment
  }
}

/*
S3 버킷은 기본적으로 외부 접근이 차단되어 있음 -> 퍼블릭 접근 가능하도록 설정
*/
resource "aws_s3_bucket_public_access_block" "this" {
    bucket = aws_s3_bucket.this.id

    block_public_acls       = false
    block_public_policy     = false
    ignore_public_acls      = false
    restrict_public_buckets = false
}

/*
S3 bucket에 Policy를 적용하겠다
*/
resource "aws_s3_bucket_policy" "this" {
    bucket = aws_s3_bucket.this.id

    depends_on = [
        aws_s3_bucket_public_access_block.this
    ]

    policy = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "Statement1",
            "Effect": "Allow",
            "Principal": "*",
            "Action": [
                "s3:GetObject",
                "s3:PutObject"
            ],
            "Resource": "arn:aws:s3:::${aws_s3_bucket.this.bucket}/*"
        }
    ]
}
POLICY
}

/*
CORS 정책 적용
*/
resource "aws_s3_bucket_cors_configuration" "this" {
  bucket = aws_s3_bucket.this.id

  cors_rule {
    allowed_headers = ["*", "Content-Type"]
    allowed_methods = ["GET", "PUT", "POST"]
    allowed_origins = ["*"]
    expose_headers  = ["ETag", "Access-Control-Allow-Origin"]
  }
}