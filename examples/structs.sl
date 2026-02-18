// Demonstrates: struct declarations, scalar struct usage, and SoA arrays of structs

struct Vec2 {
    x: f64,
    y: f64,
}

struct Particle {
    x: f64,
    y: f64,
    vx: f64,
    vy: f64,
}

fn main() -> i32 {
    // --- Scalar struct ---
    let p: Vec2 = Vec2 { x: 3.0, y: 4.0 };
    print(p.x);   // 3.000000
    print(p.y);   // 4.000000

    // Mutable scalar struct with field assignment
    let mut q: Vec2 = Vec2 { x: 0.0, y: 0.0 };
    q.x = 10.0;
    q.y = 20.0;
    print(q.x);   // 10.000000

    // --- SoA array of structs ---
    // Memory layout: { [4 x f64] xs, [4 x f64] ys } — NOT [{f64,f64}, {f64,f64}, ...]
    // This means xs are stored contiguously in memory → auto-vectorisable loops
    let mut points: [Vec2; 4];
    points[0].x = 1.0;  points[0].y = 10.0;
    points[1].x = 2.0;  points[1].y = 20.0;
    points[2].x = 3.0;  points[2].y = 30.0;
    points[3].x = 4.0;  points[3].y = 40.0;

    // Sum all x values — this loop touches xs[0..4] consecutively (cache-friendly)
    let mut sum_x: f64 = 0.0;
    for i in 0..4 {
        sum_x = sum_x + points[i].x;
    }
    print(sum_x);   // 10.000000  (1+2+3+4)

    // Sum all y values
    let mut sum_y: f64 = 0.0;
    for i in 0..4 {
        sum_y = sum_y + points[i].y;
    }
    print(sum_y);   // 100.000000  (10+20+30+40)

    // --- Multi-field SoA (Particle simulation) ---
    let mut particles: [Particle; 3];
    particles[0].x = 0.0;   particles[0].y = 0.0;
    particles[0].vx = 1.0;  particles[0].vy = 0.5;
    particles[1].x = 1.0;   particles[1].y = 2.0;
    particles[1].vx = -1.0; particles[1].vy = 0.0;
    particles[2].x = 5.0;   particles[2].y = 5.0;
    particles[2].vx = 0.0;  particles[2].vy = -1.0;

    // Advance all particles by one time step: x += vx, y += vy
    for i in 0..3 {
        particles[i].x = particles[i].x + particles[i].vx;
        particles[i].y = particles[i].y + particles[i].vy;
    }

    print(particles[0].x);   // 1.000000
    print(particles[1].x);   // 0.000000
    print(particles[2].y);   // 4.000000

    return 0;
}
