"""
Trace les regressions batterie/nRF et nRF/temps avec Bokeh.

Usage:
    python3 python/battery_regression_plot.py

Dans une session interactive VSCode, les graphiques s'affichent inline
et un fichier HTML autonome est genere.
"""

from __future__ import annotations

import json
from datetime import datetime, timedelta
from pathlib import Path

from bokeh.io import output_notebook, save, show
from bokeh.layouts import column
from bokeh.models import Div, Label, LinearAxis, Range1d, Span
from bokeh.plotting import figure


CALIBRATION_POINTS = [
    (4.07, 381),
    (3.88, 363),
    (3.75, 351),
    (3.63, 342),
]
JSONL_PATH = Path(__file__).with_name("lora-receive.jsonl")
HTML_PATH = Path(__file__).with_name("battery_regression_plot.html")
TARGET_NRF_VOLT = 318
DERIVATIVE_SMOOTHING_WINDOW = 21
SECOND_PLOT_GUIDE_COUNTER = 1500


def linear_regression(xs: list[float], ys: list[float]) -> tuple[float, float]:
    """Retourne la pente et l'ordonnee a l'origine de y = a*x + b."""
    n = len(xs)
    sx = sum(xs)
    sy = sum(ys)
    sxy = sum(x * y for x, y in zip(xs, ys))
    sx2 = sum(x * x for x in xs)
    slope = (n * sxy - sx * sy) / (n * sx2 - sx * sx)
    intercept = (sy - slope * sx) / n
    return slope, intercept


def regression_sse(xs: list[float], ys: list[float], slope: float, intercept: float) -> float:
    """Retourne la somme des erreurs quadratiques pour une regression."""
    return sum((y - (slope * x + intercept)) ** 2 for x, y in zip(xs, ys))


def moving_average(values: list[float], window_size: int) -> list[float]:
    """Lisse une serie avec une moyenne glissante centree."""
    if window_size <= 1 or len(values) < 3:
        return list(values)

    radius = window_size // 2
    smoothed = []
    for index in range(len(values)):
        start = max(0, index - radius)
        end = min(len(values), index + radius + 1)
        window = values[start:end]
        smoothed.append(sum(window) / len(window))
    return smoothed


def find_linear_region_end(minutes: list[float], volts: list[float]) -> int:
    """Trouve la fin de la zone lineaire en cherchant la meilleure rupture en deux droites."""
    min_points = max(20, len(minutes) // 20)
    best_index = len(minutes) - min_points
    best_score = float("inf")

    for split_index in range(min_points, len(minutes) - min_points):
        left_minutes = minutes[:split_index]
        left_volts = volts[:split_index]
        right_minutes = minutes[split_index:]
        right_volts = volts[split_index:]

        left_slope, left_intercept = linear_regression(left_minutes, left_volts)
        right_slope, right_intercept = linear_regression(right_minutes, right_volts)
        score = regression_sse(left_minutes, left_volts, left_slope, left_intercept)
        score += regression_sse(right_minutes, right_volts, right_slope, right_intercept)

        if score < best_score:
            best_score = score
            best_index = split_index

    return best_index


def load_rows(path: Path) -> list[tuple[datetime, int]]:
    """Charge les mesures temporelles depuis le JSONL."""
    rows = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        obj = json.loads(line)
        if "CURRENT TIME" not in obj or "volt" not in obj:
            continue
        dt = datetime.strptime(obj["CURRENT TIME"], "%Y-%m-%d %H:%M:%S")
        rows.append((dt, int(obj["volt"])))
    return sorted(rows, key=lambda row: row[0])


def load_counter_rows(path: Path) -> list[tuple[datetime, int, int]]:
    """Charge les mesures temporelles avec COUNTER.VALUE depuis le JSONL."""
    rows = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        obj = json.loads(line)
        if "CURRENT TIME" not in obj or "volt" not in obj:
            continue
        counter = obj.get("COUNTER", {}).get("VALUE")
        if counter is None:
            continue
        dt = datetime.strptime(obj["CURRENT TIME"], "%Y-%m-%d %H:%M:%S")
        rows.append((dt, int(obj["volt"]), int(counter)))
    return sorted(rows, key=lambda row: row[0])


def get_counter_fit(path: Path) -> tuple[float, float]:
    """Calcule la relation lineaire COUNTER.VALUE = a * Vgpio + b sur la zone lineaire."""
    counter_rows = load_counter_rows(path)
    counter_minutes = [
        (dt - counter_rows[0][0]).total_seconds() / 60.0 for dt, _, _ in counter_rows
    ]
    linear_end_index = find_linear_region_end(counter_minutes, [volt for _, volt, _ in counter_rows])
    linear_nrf_values = [volt for _, volt, _ in counter_rows[:linear_end_index]]
    linear_counter_values = [counter for _, _, counter in counter_rows[:linear_end_index]]
    return linear_regression(linear_nrf_values, linear_counter_values)


def get_battery_fit_mv() -> tuple[float, float]:
    """Calcule la relation lineaire Vmultimeter(mV) = a * Vgpio + b."""
    battery_volts = [x for x, _ in CALIBRATION_POINTS]
    nrf_values = [y for _, y in CALIBRATION_POINTS]
    slope, intercept = linear_regression(battery_volts, nrf_values)
    return 1000.0 / slope, -1000.0 * intercept / slope


def build_calibration_plot() -> tuple[figure, float, float]:
    """Cree le graphique nRF -> tension batterie."""
    battery_volts = [x for x, _ in CALIBRATION_POINTS]
    battery_millivolts = [x * 1000.0 for x in battery_volts]
    nrf_values = [y for _, y in CALIBRATION_POINTS]
    slope, intercept = linear_regression(battery_volts, nrf_values)
    inverse_slope_mv, inverse_intercept_mv = get_battery_fit_mv()

    x_min = min(nrf_values) - 5
    x_max = max(nrf_values) + 5
    reg_xs = [x_min, x_max]
    reg_ys = [inverse_slope_mv * x + inverse_intercept_mv for x in reg_xs]
    counter_slope, counter_intercept = get_counter_fit(JSONL_PATH)
    counter_start = counter_slope * x_min + counter_intercept
    counter_end = counter_slope * x_max + counter_intercept

    plot = figure(
        title="nRF Power Consumption Study with a 3,7V 150mAh LiPo Battery — Vmultimeter vs Vgpio",
        x_axis_label="Vgpio ()",
        y_axis_label="Vmultimeter (mV)",
        width=1100,
        height=360,
        tools="pan,wheel_zoom,box_zoom,reset,save",
    )
    plot.scatter(
        nrf_values,
        battery_millivolts,
        size=10,
        color="#0b6e4f",
        legend_label="Vmultimeter vs Vgpio",
    )
    plot.line(reg_xs, reg_ys, line_width=3, color="#c1121f", legend_label="Fit")
    plot.add_layout(
        Span(location=TARGET_NRF_VOLT, dimension="height", line_dash="dashed", line_color="#1d3557")
    )
    plot.add_layout(
        Label(
            x=x_min,
            y=max(battery_millivolts) + 20,
            text=f"Vmultimeter = {inverse_slope_mv:.3f} * Vgpio + {inverse_intercept_mv:.0f}",
            text_font_size="11pt",
        )
    )
    plot.extra_x_ranges = {"counter": Range1d(start=counter_start, end=counter_end)}
    plot.add_layout(LinearAxis(x_range_name="counter", axis_label="MSG COUNT"), "above")
    plot.legend.location = "bottom_right"
    return plot, slope, intercept


def format_duration(duration: timedelta) -> str:
    """Formate une duree de maniere lisible."""
    total_seconds = int(duration.total_seconds())
    hours, remainder = divmod(total_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{hours:d} h {minutes:02d} min {seconds:02d} s"


def build_time_plot() -> tuple[figure, float, float, datetime, datetime, datetime, timedelta, datetime, float]:
    """Cree le graphique temporel principal et retourne les infos utiles."""
    counter_rows = load_counter_rows(JSONL_PATH)
    if len(counter_rows) < 2:
        raise ValueError("Pas assez de points dans le JSONL pour calculer une regression.")

    start_dt = counter_rows[0][0]
    end_dt = counter_rows[-1][0]
    measured_duration = end_dt - start_dt
    times = [dt for dt, _, _ in counter_rows]
    volts = [volt for _, volt, _ in counter_rows]
    counters = [counter for _, _, counter in counter_rows]
    minutes = [(dt - start_dt).total_seconds() / 60.0 for dt in times]
    linear_end_index = find_linear_region_end(minutes, volts)
    linear_minutes = minutes[:linear_end_index]
    linear_volts = volts[:linear_end_index]
    linear_counters = counters[:linear_end_index]
    linear_end_minute = linear_minutes[-1]
    linear_end_dt = times[linear_end_index - 1]
    linear_end_counter = linear_counters[-1]

    slope, intercept = linear_regression(linear_minutes, linear_volts)
    target_minutes = (TARGET_NRF_VOLT - intercept) / slope
    target_dt = start_dt + timedelta(minutes=target_minutes)
    counter_from_minute_slope, counter_from_minute_intercept = linear_regression(linear_minutes, linear_counters)
    target_counter = counter_from_minute_slope * target_minutes + counter_from_minute_intercept
    volt_from_counter_slope, volt_from_counter_intercept = linear_regression(linear_counters, linear_volts)
    second_plot_guide_volt = (
        volt_from_counter_slope * SECOND_PLOT_GUIDE_COUNTER + volt_from_counter_intercept
    )

    regression_counters = linear_counters + [target_counter]
    regression_volts = [
        volt_from_counter_slope * counter + volt_from_counter_intercept for counter in regression_counters
    ]
    x_min = min(counters) - 20
    x_max = max(max(counters), target_counter) + 20
    minute_from_counter_slope = (minutes[-1] - minutes[0]) / (counters[-1] - counters[0])
    minute_from_counter_intercept = minutes[0] - minute_from_counter_slope * counters[0]
    time_x_min = minute_from_counter_slope * x_min + minute_from_counter_intercept
    time_x_max = minute_from_counter_slope * x_max + minute_from_counter_intercept
    y_min = 325
    y_max = 400

    plot_minutes = figure(
        title="nRF Power Consumption Study with a 3,7V 150mAh LiPo Battery — Vgpio vs Msg Count",
        x_axis_label="MSG COUNT",
        y_axis_label="Vgpio ()",
        width=1100,
        height=360,
        tools="pan,wheel_zoom,box_zoom,reset,save",
        x_range=Range1d(start=x_min, end=x_max),
        y_range=Range1d(start=y_min, end=y_max),
    )
    plot_minutes.scatter(counters, volts, size=6, color="#6d597a", legend_label="Vgpio vs Msg Count")
    plot_minutes.line(
        regression_counters,
        regression_volts,
        line_width=3,
        color="#b56576",
        legend_label="Fit",
    )
    plot_minutes.add_layout(
        Span(location=second_plot_guide_volt, dimension="width", line_dash="dashed", line_color="#1d3557")
    )
    plot_minutes.add_layout(
        Span(location=SECOND_PLOT_GUIDE_COUNTER, dimension="height", line_dash="dashed", line_color="#1d3557")
    )
    plot_minutes.extra_x_ranges = {"time": Range1d(start=time_x_min, end=time_x_max)}
    plot_minutes.add_layout(LinearAxis(x_range_name="time", axis_label="Time (min)"), "above")
    battery_slope_mv, battery_intercept_mv = get_battery_fit_mv()
    battery_y_min = battery_slope_mv * y_min + battery_intercept_mv
    battery_y_max = battery_slope_mv * y_max + battery_intercept_mv
    plot_minutes.extra_y_ranges = {"vbat": Range1d(start=battery_y_min, end=battery_y_max)}
    plot_minutes.add_layout(LinearAxis(y_range_name="vbat", axis_label="Vmultimeter (mV)"), "right")
    plot_minutes.legend.location = "top_right"

    return (
        plot_minutes,
        slope,
        intercept,
        target_dt,
        start_dt,
        end_dt,
        measured_duration,
        linear_end_dt,
        linear_end_minute,
    )


def build_derivative_plot() -> figure:
    """Cree le graphique de derivee dVgpio/dCOUNTER."""
    counter_rows = load_counter_rows(JSONL_PATH)
    if len(counter_rows) < 2:
        raise ValueError("Pas assez de points dans le JSONL pour calculer une derivee.")

    start_dt = counter_rows[0][0]
    times = [dt for dt, _, _ in counter_rows]
    volts = [volt for _, volt, _ in counter_rows]
    counters = [counter for _, _, counter in counter_rows]
    smoothed_volts = moving_average([float(volt) for volt in volts], DERIVATIVE_SMOOTHING_WINDOW)
    minutes = [(dt - start_dt).total_seconds() / 60.0 for dt in times]
    linear_end_index = find_linear_region_end(minutes, volts)
    linear_end_counter = counters[linear_end_index - 1]

    derivative_counters = []
    derivative_values = []
    for index in range(len(counters) - 1):
        delta_counter = counters[index + 1] - counters[index]
        if delta_counter == 0:
            continue
        midpoint_counter = 0.5 * (counters[index + 1] + counters[index])
        derivative = (smoothed_volts[index + 1] - smoothed_volts[index]) / delta_counter
        derivative_counters.append(midpoint_counter)
        derivative_values.append(derivative)

    y_min = min(derivative_values) - 0.1
    y_max = max(derivative_values) + 0.1
    x_min = min(counters) - 20
    x_max = max(counters) + 20

    plot_derivative = figure(
        title="dVgpio / dCOUNTER",
        x_axis_label="MSG COUNT",
        y_axis_label="dVgpio/dCOUNTER",
        width=1100,
        height=280,
        tools="pan,wheel_zoom,box_zoom,reset,save",
        x_range=Range1d(start=x_min, end=x_max),
        y_range=Range1d(start=y_min, end=y_max),
    )
    plot_derivative.line(
        derivative_counters,
        derivative_values,
        line_width=2,
        color="#264653",
        legend_label=f"Derivee lissée (fenetre {DERIVATIVE_SMOOTHING_WINDOW})",
    )
    plot_derivative.add_layout(
        BoxAnnotation(
            left=counters[0],
            right=linear_end_counter,
            fill_color="#a8dadc",
            fill_alpha=0.15,
        )
    )
    plot_derivative.add_layout(
        Span(location=0, dimension="width", line_dash="dashed", line_color="#6c757d")
    )
    plot_derivative.add_layout(
        Span(location=linear_end_counter, dimension="height", line_dash="dashed", line_color="#2a9d8f")
    )
    plot_derivative.legend.location = "bottom_left"
    return plot_derivative


def main() -> None:
    output_notebook()

    calibration_plot, calibration_slope, calibration_intercept = build_calibration_plot()
    (
        minutes_plot,
        time_slope,
        time_intercept,
        target_dt,
        start_dt,
        end_dt,
        measured_duration,
        linear_end_dt,
        linear_end_minute,
    ) = build_time_plot()
    battery_mv_slope = 1000.0 / calibration_slope
    battery_mv_intercept = -1000.0 * calibration_intercept / calibration_slope

    calibration_summary = Div(
        text=(
            f"<p><b>Equation de conversion batterie:</b> "
            f"Tension batterie (mV) = ({battery_mv_slope:.3f} * mesure Vgpio) + {battery_mv_intercept:.0f}</p>"
        ),
        width=1100,
    )

    summary = Div(
        text=(
            f"<p><b>Debut:</b> {start_dt:%Y-%m-%d %H:%M:%S}<br>"
            f"<b>Fin:</b> {end_dt:%Y-%m-%d %H:%M:%S}<br>"
            f"<b>Duree:</b> {format_duration(measured_duration)}<br>"
            f"<b>Fin zone lineaire:</b> {linear_end_dt:%Y-%m-%d %H:%M:%S} ({linear_end_minute:.1f} min)<br>"
            f"<b>Heure extrapolee pour Vgpio = {TARGET_NRF_VOLT}:</b> {target_dt:%Y-%m-%d %H:%M:%S}</p>"
        ),
        width=1100,
    )
    layout = column(summary, calibration_summary, calibration_plot, minutes_plot)

    print(f"Calibration: Vgpio = {calibration_slope:.6f} * V + {calibration_intercept:.6f}")
    print(f"Decharge: Vgpio = {time_slope:.6f} * minutes + {time_intercept:.6f}")
    print(f"Heure de debut: {start_dt:%Y-%m-%d %H:%M:%S}")
    print(f"Heure de fin:   {end_dt:%Y-%m-%d %H:%M:%S}")
    print(f"Duree:          {format_duration(measured_duration)}")
    print(f"Fin zone lineaire: {linear_end_dt:%Y-%m-%d %H:%M:%S} ({linear_end_minute:.1f} min)")
    print(f"Heure extrapolee pour Vgpio = {TARGET_NRF_VOLT}: {target_dt:%Y-%m-%d %H:%M:%S}")
    print(f"HTML ecrit dans: {HTML_PATH}")

    show(layout)
    save(layout, filename=str(HTML_PATH), title="Battery Regression Plot")


if __name__ == "__main__":
    main()
