use http::Request;
use httparse;
use std::env;

fn generate_request(size: &str) -> Vec<u8> {
    let mut base = "GET /api/v1/users/123 HTTP/1.1\r\nHost: api.example.com\r\n".to_string();

    match size {
        "sm" => {
            base.push_str("User-Agent: Benchmark/1.0\r\n");
            base.push_str("Accept: application/json\r\n");
            base.push_str("Connection: keep-alive\r\n");
        }
        "lg" => {
            for i in 0..50 {
                base.push_str(&format!("X-Custom-Header-{}: some-value-{}\r\n", i, i));
            }
        }
        "xl" => {
            for i in 0..1000 {
                base.push_str(&format!("X-Large-Header-{}: {}\r\n", i, "X".repeat(50)));
            }
        }
        _ => {}
    }

    base.push_str("\r\n");
    base.into_bytes()
}

fn parse_with_httparse(raw: &[u8]) -> Result<Request<Vec<u8>>, Box<dyn std::error::Error>> {
    let mut headers = vec![httparse::EMPTY_HEADER; 1024];
    let mut req = httparse::Request::new(&mut headers);

    let body_offset = req.parse(raw)?.unwrap();

    let mut builder = Request::builder()
        .method(req.method.unwrap())
        .uri(req.path.unwrap());

    for header in req.headers {
        builder = builder.header(header.name, header.value);
    }

    Ok(builder.body(raw[body_offset..].to_vec())?)
}

fn parser_benchmark(size: &str, iterations: usize) {
    let request_data = generate_request(size);
    let mut total_header_len = 0usize;

    for _ in 0..iterations {
        let req = parse_with_httparse(&request_data).unwrap();
        for (name, value) in req.headers() {
            total_header_len += name.as_str().len() + value.len();
        }
    }

    println!("{}", total_header_len);
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: parser_bench <sm|lg|xl> [iterations]");
        std::process::exit(1);
    }

    let size = &args[1];
    let iterations: usize = if args.len() > 2 {
        args[2].parse().unwrap_or(100000)
    } else {
        100000
    };

    parser_benchmark(size, iterations);
}
