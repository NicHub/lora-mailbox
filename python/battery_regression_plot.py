"""
Trace les regressions batterie/nRF et nRF/temps avec Bokeh.

Usage:
    python3 python/battery_regression_plot.py

Dans une session interactive VSCode, les graphiques s'affichent inline.
"""

from __future__ import annotations

import json
from datetime import datetime, timedelta
from pathlib import Path

from bokeh.io import output_notebook, show
from bokeh.layouts import column
from bokeh.models import DatetimeTickFormatter, Label, Span
from bokeh.plotting import figure


CALIBRATION_POINTS = [
    (4.07, 381),
    (3.88, 363),
    (3.75, 351),
    (3.63, 342),
]
JSONL_PATH = Path(__file__).with_name("lora-receive.jsonl")
TARGET_NRF_VOLT = 318


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
    return rows


def build_calibration_plot() -> tuple[figure, float, float]:
    """Cree le graphique voltmetre -> nRF."""
    xs = [x for x, _ in CALIBRATION_POINTS]
    ys = [y for _, y in CALIBRATION_POINTS]
    slope, intercept = linear_regression(xs, ys)

    x_min = min(xs) - 0.05
    x_max = max(xs) + 0.05
    reg_xs = [x_min, x_max]
    reg_ys = [slope * x + intercept for x in reg_xs]

    plot = figure(
        title="Tension batterie vs mesure nRF",
        x_axis_label="Tension batterie au voltmetre (V)",
        y_axis_label="Mesure nRF",
        width=1100,
        height=360,
        tools="pan,wheel_zoom,box_zoom,reset,save",
    )
    plot.scatter(xs, ys, size=10, color="#0b6e4f", legend_label="Mesures")
    plot.line(reg_xs, reg_ys, line_width=3, color="#c1121f", legend_label="Regression lineaire")
    plot.add_layout(
        Span(location=TARGET_NRF_VOLT, dimension="width", line_dash="dashed", line_color="#1d3557")
    )
    plot.add_layout(
        Label(
            x=x_min,
            y=max(ys) + 2,
            text=f"nRF = {slope:.2f} * V + {intercept:.2f}",
            text_font_size="11pt",
        )
    )
    plot.legend.location = "top_left"
    return plot, slope, intercept


def build_time_plots() -> tuple[figure, figure, float, float, datetime]:
    """Cree les graphiques temporels et retourne les infos de regression."""
    rows = load_rows(JSONL_PATH)
    if len(rows) < 2:
        raise ValueError("Pas assez de points dans le JSONL pour calculer une regression.")

    start_dt = rows[0][0]
    times = [dt for dt, _ in rows]
    volts = [volt for _, volt in rows]
    minutes = [(dt - start_dt).total_seconds() / 60.0 for dt in times]

    slope, intercept = linear_regression(minutes, volts)
    target_minutes = (TARGET_NRF_VOLT - intercept) / slope
    target_dt = start_dt + timedelta(minutes=target_minutes)

    regression_times = times + [target_dt]
    regression_minutes = minutes + [target_minutes]
    regression_volts = [slope * minute + intercept for minute in regression_minutes]

    plot_time = figure(
        title="Tension nRF vs heure",
        x_axis_type="datetime",
        x_axis_label="Heure",
        y_axis_label="Tension nRF",
        width=1100,
        height=360,
        tools="pan,wheel_zoom,box_zoom,reset,save",
    )
    plot_time.scatter(times, volts, size=6, color="#457b9d", legend_label="Mesures")
    plot_time.line(
        regression_times,
        regression_volts,
        line_width=3,
        color="#e63946",
        legend_label="Regression lineaire",
    )
    plot_time.add_layout(
        Span(location=TARGET_NRF_VOLT, dimension="width", line_dash="dashed", line_color="#1d3557")
    )
    plot_time.add_layout(
        Span(location=target_dt.timestamp() * 1000, dimension="height", line_dash="dashed", line_color="#f4a261")
    )
    plot_time.add_layout(
        Label(
            x=times[0],
            y=max(volts) + 2,
            text=f"passage a {TARGET_NRF_VOLT}: {target_dt:%Y-%m-%d %H:%M:%S}",
            text_font_size="11pt",
        )
    )
    plot_time.xaxis.formatter = DatetimeTickFormatter(hours="%H:%M", minutes="%H:%M")
    plot_time.legend.location = "bottom_left"

    plot_minutes = figure(
        title="Tension nRF vs minutes depuis le debut",
        x_axis_label="Minutes depuis la premiere mesure",
        y_axis_label="Tension nRF",
        width=1100,
        height=360,
        tools="pan,wheel_zoom,box_zoom,reset,save",
    )
    plot_minutes.scatter(minutes, volts, size=6, color="#6d597a", legend_label="Mesures")
    plot_minutes.line(
        regression_minutes,
        regression_volts,
        line_width=3,
        color="#b56576",
        legend_label="Regression lineaire",
    )
    plot_minutes.add_layout(
        Span(location=TARGET_NRF_VOLT, dimension="width", line_dash="dashed", line_color="#1d3557")
    )
    plot_minutes.add_layout(
        Span(location=target_minutes, dimension="height", line_dash="dashed", line_color="#f4a261")
    )
    plot_minutes.add_layout(
        Label(
            x=minutes[0],
            y=max(volts) + 2,
            text=f"volt = {slope:.6f} * minutes + {intercept:.6f}",
            text_font_size="11pt",
        )
    )
    plot_minutes.legend.location = "bottom_left"

    return plot_time, plot_minutes, slope, intercept, target_dt


def main() -> None:
    output_notebook()

    calibration_plot, calibration_slope, calibration_intercept = build_calibration_plot()
    time_plot, minutes_plot, time_slope, time_intercept, target_dt = build_time_plots()

    print(f"Calibration: nRF = {calibration_slope:.6f} * V + {calibration_intercept:.6f}")
    print(f"Decharge: nRF = {time_slope:.6f} * minutes + {time_intercept:.6f}")
    print(f"Heure extrapolee pour nRF = {TARGET_NRF_VOLT}: {target_dt:%Y-%m-%d %H:%M:%S}")

    show(column(calibration_plot, time_plot, minutes_plot))


if __name__ == "__main__":
    main()
