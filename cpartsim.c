#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// CONSTANTS MUST BE THE SAME AS IN THE PYTHON CODE
static int    N_PARTICLES   = 0;
static double DT            = 0.02;
static double FRICTION_HALF = 0.04;
static double R_MAX         = 0.1;
static int    COLORS        = 6;
static double FORCE_FACTOR  = 3.0;

static double friction_factor = 0.0;
static double CELL_SIZE       = 0.0;
static int    grid_size       = 0;

static double *matrix = NULL;

typedef struct {
    double x, y;
    double vx, vy;
    int    color;
} Particle;

static Particle *particles = NULL;

typedef struct {
    int start_idx;
    int count;
} CellInfo;

static CellInfo *cells   = NULL;
static int      *indices = NULL;

static unsigned int sim_seed = 0;

static double rand01(void) {
    return (double)rand() / (double)RAND_MAX;
}

static double compute_force(double norm_r, double attraction) {
    double beta = 0.3;
    if (norm_r < beta) {
        return (norm_r / beta) - 1.0;
    }
    else if (norm_r < 1.0) {
        return attraction * (1.0 - fabs(2.0 * norm_r - 1.0 - beta) / (1.0 - beta));
    }
    else {
        return 0.0;
    }
}

static void build_grid(void) {
    int total_cells = grid_size * grid_size;

    for (int i = 0; i < total_cells; i++) {
        cells[i].count = 0;
    }

    for (int i = 0; i < N_PARTICLES; i++) {
        double x = particles[i].x;
        double y = particles[i].y;
        int gx = (int)(x * (double)grid_size);
        int gy = (int)(y * (double)grid_size);
        if (gx < 0)       gx = 0;
        else if (gx >= grid_size) gx = grid_size - 1;
        if (gy < 0)       gy = 0;
        else if (gy >= grid_size) gy = grid_size - 1;
        int cell_id = gy * grid_size + gx;
        cells[cell_id].count++;
    }

    int running_sum = 0;
    for (int i = 0; i < total_cells; i++) {
        int c = cells[i].count;
        cells[i].start_idx = running_sum;
        running_sum += c;
    }

    int *cursor = (int*)calloc(total_cells, sizeof(int));
    if (!cursor) {
        return;
    }

    for (int i = 0; i < N_PARTICLES; i++) {
        double x = particles[i].x;
        double y = particles[i].y;
        int gx = (int)(x * (double)grid_size);
        int gy = (int)(y * (double)grid_size);
        if (gx < 0)       gx = 0;
        else if (gx >= grid_size) gx = grid_size - 1;
        if (gy < 0)       gy = 0;
        else if (gy >= grid_size) gy = grid_size - 1;
        int cell_id = gy * grid_size + gx;

        int insert_pos = cells[cell_id].start_idx + cursor[cell_id];
        indices[insert_pos] = i;
        cursor[cell_id]++;
    }

    free(cursor);
}

static void do_update_frame(void) {
    build_grid();

    for (int i = 0; i < N_PARTICLES; i++) {
        Particle *p1 = &particles[i];
        double x1 = p1->x;
        double y1 = p1->y;
        double vx = p1->vx * friction_factor;
        double vy = p1->vy * friction_factor;
        int c1 = p1->color;

        int gx = (int)(x1 * (double)grid_size);
        int gy = (int)(y1 * (double)grid_size);
        if (gx < 0)       gx = 0;
        else if (gx >= grid_size) gx = grid_size - 1;
        if (gy < 0)       gy = 0;
        else if (gy >= grid_size) gy = grid_size - 1;

        double total_fx = 0.0;
        double total_fy = 0.0;

        for (int dx_cell = -1; dx_cell <= 1; dx_cell++) {
            int ngx = gx + dx_cell;
            if (ngx < 0)       ngx += grid_size;
            else if (ngx >= grid_size) ngx -= grid_size;

            for (int dy_cell = -1; dy_cell <= 1; dy_cell++) {
                int ngy = gy + dy_cell;
                if (ngy < 0)       ngy += grid_size;
                else if (ngy >= grid_size) ngy -= grid_size;

                int cell_id = ngy * grid_size + ngx;
                int bucket_start = cells[cell_id].start_idx;
                int bucket_count = cells[cell_id].count;

                for (int k = 0; k < bucket_count; k++) {
                    int j = indices[bucket_start + k];
                    if (j == i) continue;
                    Particle *p2 = &particles[j];

                    double dx = p2->x - x1;
                    double dy = p2->y - y1;
                    if (dx >  0.5) dx -= 1.0;
                    else if (dx < -0.5) dx += 1.0;
                    if (dy >  0.5) dy -= 1.0;
                    else if (dy < -0.5) dy += 1.0;

                    double r = sqrt(dx*dx + dy*dy);
                    if (r > 0.0 && r < R_MAX) {
                        double norm_r = r / R_MAX;
                        int c2 = p2->color;
                        double attr = matrix[c1 * COLORS + c2];
                        double f = compute_force(norm_r, attr);
                        double inv_r = 1.0 / r;
                        total_fx += (dx * inv_r) * f;
                        total_fy += (dy * inv_r) * f;
                    }
                }
            }
        }

        total_fx *= (R_MAX * FORCE_FACTOR);
        total_fy *= (R_MAX * FORCE_FACTOR);

        p1->vx = vx + total_fx * DT;
        p1->vy = vy + total_fy * DT;
    }

    for (int i = 0; i < N_PARTICLES; i++) {
        Particle *p = &particles[i];
        p->x += p->vx * DT;
        p->y += p->vy * DT;
        if (p->x < 0.0)       p->x += 1.0;
        else if (p->x >= 1.0) p->x -= 1.0;
        if (p->y < 0.0)       p->y += 1.0;
        else if (p->y >= 1.0) p->y -= 1.0;
    }
}

// Python wrappers

static PyObject *py_init_simulation(PyObject *self, PyObject *args) {
    int n;
    int seed_in = -1;

    if (!PyArg_ParseTuple(args, "i|i", &n, &seed_in)) {
        return NULL;
    }
    N_PARTICLES = n;

    if (seed_in < 0) {
        sim_seed = (unsigned int)time(NULL);
    } else {
        sim_seed = (unsigned int)seed_in;
    }
    srand(sim_seed);

    friction_factor = pow(0.5, (DT / FRICTION_HALF));
    CELL_SIZE       = R_MAX;
    grid_size       = (int)(1.0 / CELL_SIZE);

    if (particles) {
        free(particles);
        particles = NULL;
    }
    particles = (Particle*)malloc(sizeof(Particle) * N_PARTICLES);
    if (!particles) {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate particles array");
        return NULL;
    }

    int total_cells = grid_size * grid_size;
    if (cells) {
        free(cells);
        cells = NULL;
    }
    cells = (CellInfo*)malloc(sizeof(CellInfo) * total_cells);
    if (!cells) {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate cells array");
        return NULL;
    }

    for (int i = 0; i < total_cells; i++) {
        cells[i].start_idx = 0;
        cells[i].count     = 0;
    }

    if (indices) {
        free(indices);
        indices = NULL;
    }
    indices = (int*)malloc(sizeof(int) * N_PARTICLES);
    if (!indices) {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate indices array");
        return NULL;
    }

    if (matrix) {
        free(matrix);
        matrix = NULL;
    }
    matrix = (double*)malloc(sizeof(double) * COLORS * COLORS);
    if (!matrix) {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate matrix array");
        return NULL;
    }

    for (int i = 0; i < COLORS * COLORS; i++) {
        matrix[i] = (rand01() * 2.0) - 1.0;
    }

    for (int i = 0; i < N_PARTICLES; i++) {
        particles[i].x     = rand01();
        particles[i].y     = rand01();
        particles[i].vx    = 0.0;
        particles[i].vy    = 0.0;
        particles[i].color = rand() % COLORS;
    }

    Py_RETURN_NONE;
}

static PyObject *py_update_frame(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    if (!particles) {
        PyErr_SetString(PyExc_RuntimeError, "Simulation not initialized. Call init_simulation(n) first.");
        return NULL;
    }
    do_update_frame();
    Py_RETURN_NONE;
}

static PyObject *py_get_positions(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    if (!particles) {
        PyErr_SetString(PyExc_RuntimeError, "Simulation not initialized. Call init_simulation(n) first.");
        return NULL;
    }

    PyObject *list_x = PyList_New(N_PARTICLES);
    PyObject *list_y = PyList_New(N_PARTICLES);
    PyObject *list_c = PyList_New(N_PARTICLES);
    if (!list_x || !list_y || !list_c) {
        Py_XDECREF(list_x);
        Py_XDECREF(list_y);
        Py_XDECREF(list_c);
        PyErr_SetString(PyExc_MemoryError, "Could not allocate Python lists for positions.");
        return NULL;
    }

    for (int i = 0; i < N_PARTICLES; i++) {
        Particle *p = &particles[i];
        PyObject *px = PyFloat_FromDouble(p->x);
        PyObject *py = PyFloat_FromDouble(p->y);
        PyObject *pc = PyLong_FromLong(p->color);
        if (!px || !py || !pc) {
            Py_XDECREF(px);
            Py_XDECREF(py);
            Py_XDECREF(pc);
            Py_DECREF(list_x);
            Py_DECREF(list_y);
            Py_DECREF(list_c);
            PyErr_SetString(PyExc_MemoryError, "Could not build position/color list items.");
            return NULL;
        }
        PyList_SET_ITEM(list_x, i, px);
        PyList_SET_ITEM(list_y, i, py);
        PyList_SET_ITEM(list_c, i, pc);
    }

    return PyTuple_Pack(3, list_x, list_y, list_c);
}

static PyObject *py_get_seed(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    return PyLong_FromUnsignedLong(sim_seed);
}

static void cleanup_simulation(void) {
    if (particles) {
        free(particles);
        particles = NULL;
    }
    if (matrix) {
        free(matrix);
        matrix = NULL;
    }
    if (cells) {
        free(cells);
        cells = NULL;
    }
    if (indices) {
        free(indices);
        indices = NULL;
    }
}

static PyMethodDef CPartSimMethods[] = {
    {"init_simulation", (PyCFunction)py_init_simulation, METH_VARARGS,
     "Initialize the particle simulation with N particles.\n"
     "Usage:\n"
     "  init_simulation(n)            # picks a time-based seed\n"
     "  init_simulation(n, seed_val)  # uses your provided seed\n"
    },
    {"update_frame",   (PyCFunction)py_update_frame,   METH_NOARGS,
     "Advance the simulation by one time step."
    },
    {"get_positions",  (PyCFunction)py_get_positions,  METH_NOARGS,
     "Get three lists: x_coords, y_coords, color indices."
    },
    {"get_seed",       (PyCFunction)py_get_seed,       METH_NOARGS,
     "Return the unsigned int seed that was used for this simulation.\n"
     "If you called init_simulation(n) without passing a seed, this is the time-based seed.\n"
     "If you called init_simulation(n, seed_val), this returns seed_val."
    },
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef cpartsim_module = {
    PyModuleDef_HEAD_INIT,
    "cpartsim",
    "C-accelerated particle sim",
    -1,
    CPartSimMethods,
    NULL, NULL, NULL,
    (freefunc)cleanup_simulation
};

PyMODINIT_FUNC PyInit_cpartsim(void) {
    return PyModule_Create(&cpartsim_module);
}
