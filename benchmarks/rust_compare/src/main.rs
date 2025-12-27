use httparse;
use std::env;

fn generate_request(size: &str) -> Vec<u8> {
    let mut base = String::from("GET /api/v1/users/123 HTTP/1.1\r\nHost: api.example.com\r\n");

    if size == "sm" {
        base.push_str("User-Agent: Benchmark/1.0\r\n");
        base.push_str("Accept: application/json\r\n");
        base.push_str("Connection: keep-alive\r\n");
    } else if size == "lg" {
        for i in 0..50 {
            base.push_str(&format!("X-Custom-Header-{}: some-value-{}\r\n", i, i));
        }
    } else if size == "xl" {
        for i in 0..1000 {
            base.push_str(&format!("X-Large-Header-{}: {:X<50}\r\n", i, ""));
        }
    }

    base.push_str("\r\n");
    return base.into_bytes();
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: rust_compare <sm|lg|xl> [iterations]");
        return;
    }

    let size = &args[1];
    let iterations = if args.len() > 2 {
        args[2].parse().unwrap_or(100000)
    } else {
        100000
    };

    let request_data = generate_request(size);
    let mut total_header_len = 0;

    const MAX_HEADERS: usize = 2048;

    for _ in 0..iterations {
        let mut headers = [httparse::EMPTY_HEADER; MAX_HEADERS];
        let mut req = httparse::Request::new(&mut headers);

        let status = req.parse(&request_data).unwrap();

        if status.is_complete() {
            for h in req.headers {
                total_header_len += h.name.len() + h.value.len();
            }
        }
    }

    println!("{}", total_header_len);
}

